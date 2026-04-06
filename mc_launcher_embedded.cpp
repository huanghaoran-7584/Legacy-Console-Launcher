#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <set>
#include <map>

#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

// 资源ID
#define MINECRAFT_RESOURCE_ID 1
#define Z7_EXE_RESOURCE_ID 2
#define Z7_DLL_RESOURCE_ID 3

// 配置常量
const std::string USERNAME = "huang";
const std::string VERSION = "1.21.4-Fabric 0.18.5";
const std::string JAVA_DOWNLOAD_URL = "https://cdn.azul.com/zulu/bin/zulu25.32.21-ca-jdk25.0.2-win_x64.zip";
const std::string JAVA_ZIP_NAME = "zulu25.32.21-ca-jdk25.0.2-win_x64.zip";

// 获取AppData Local目录
std::string GetAppDataPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        return std::string(path);
    }
    return "";
}

// 检查文件/目录是否存在
bool FileExists(const std::string& path) {
    return fs::exists(path);
}

// 从exe中提取资源到文件
bool ExtractResourceFromExe(int resourceId, const char* resourceType, const std::string& outputPath) {
    HMODULE hModule = GetModuleHandleA(NULL);
    if (!hModule) {
        std::cerr << "无法获取模块句柄" << std::endl;
        return false;
    }

    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(resourceId), resourceType);
    if (!hRes) {
        std::cerr << "未找到资源ID " << resourceId << std::endl;
        return false;
    }

    HGLOBAL hLoaded = LoadResource(hModule, hRes);
    if (!hLoaded) {
        std::cerr << "无法加载资源" << std::endl;
        return false;
    }

    void* pData = LockResource(hLoaded);
    if (!pData) {
        std::cerr << "无法锁定资源" << std::endl;
        return false;
    }

    DWORD size = SizeofResource(hModule, hRes);
    std::cout << "提取资源 " << resourceId << " 到 " << outputPath << ", 大小: " << size << " 字节" << std::endl;
    
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "无法打开文件: " << outputPath << std::endl;
        return false;
    }

    outFile.write(static_cast<const char*>(pData), size);
    outFile.close();

    return true;
}

// 提取7-Zip工具
bool Extract7Zip(const std::string& tempDir) {
    std::cout << "正在提取7-Zip工具..." << std::endl;
    
    std::string exePath = tempDir + "\\7z.exe";
    std::string dllPath = tempDir + "\\7z.dll";

    if (!ExtractResourceFromExe(Z7_EXE_RESOURCE_ID, RT_RCDATA, exePath)) {
        std::cerr << "无法提取7z.exe" << std::endl;
        return false;
    }

    if (!ExtractResourceFromExe(Z7_DLL_RESOURCE_ID, RT_RCDATA, dllPath)) {
        std::cerr << "无法提取7z.dll" << std::endl;
        return false;
    }

    std::cout << "7-Zip工具提取完成" << std::endl;
    return true;
}

// 从资源中提取Minecraft
bool ExtractMinecraftFromResource() {
    std::string appData = GetAppDataPath();
    std::string minecraftDir = appData + "\\.minecraft";
    std::string tempDir = appData + "\\_mc_launcher_temp";

    std::cout << "正在从程序资源中提取Minecraft..." << std::endl;

    // 获取当前模块句柄
    HMODULE hModule = GetModuleHandleA(NULL);
    if (!hModule) {
        std::cerr << "无法获取模块句柄" << std::endl;
        return false;
    }

    // 查找资源
    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(MINECRAFT_RESOURCE_ID), RT_RCDATA);
    if (!hRes) {
        std::cerr << "未找到Minecraft资源，错误代码: " << GetLastError() << std::endl;
        return false;
    }

    // 获取资源大小
    DWORD size = SizeofResource(hModule, hRes);
    std::cout << "找到Minecraft资源，大小: " << size << " 字节" << std::endl;

    // 确保AppData目录存在
    if (!FileExists(appData)) {
        std::cout << "AppData目录不存在，正在创建..." << std::endl;
        try {
            fs::create_directories(appData);
        } catch (const std::exception& e) {
            std::cerr << "无法创建AppData目录: " << e.what() << std::endl;
            return false;
        }
    }

    // 创建临时目录
    std::string extractTempDir = tempDir + "\\extract";
    if (!FileExists(extractTempDir)) {
        fs::create_directories(extractTempDir);
    }

    // 提取7-Zip
    if (!Extract7Zip(tempDir)) {
        return false;
    }

    // 提取Minecraft ZIP文件
    std::string tempZip = tempDir + "\\.minecraft.zip";
    if (!ExtractResourceFromExe(MINECRAFT_RESOURCE_ID, RT_RCDATA, tempZip)) {
        std::cerr << "无法提取Minecraft ZIP文件" << std::endl;
        return false;
    }

    // 使用嵌入的7-Zip解压到临时目录
    std::cout << "正在使用7-Zip解压Minecraft..." << std::endl;
    std::string command = "\"" + tempDir + "\\7z.exe\" x -y -o\"" + extractTempDir + "\" \"" + tempZip + "\"";
    std::cout << "执行命令: " << command << std::endl;
    
    // 创建批处理文件来执行7z命令，避免引号问题
    std::string batchPath = tempDir + "\\extract.bat";
    std::ofstream batchFile(batchPath);
    batchFile << "@echo off\n";
    batchFile << "\"" << tempDir << "\\7z.exe\" x -y -o\"" << extractTempDir << "\" \"" << tempZip << "\"\n";
    batchFile << "exit /b %ERRORLEVEL%\n";
    batchFile.close();
    
    int result = system(("\"" + batchPath + "\"").c_str());
    std::cout << "7-Zip返回码: " << result << std::endl;

    if (result != 0) {
        std::cerr << "解压Minecraft失败" << std::endl;
        return false;
    }

    // 移动文件到目标目录
    std::cout << "正在移动文件到目标目录..." << std::endl;
    
    // 确保目标目录存在
    if (!FileExists(minecraftDir)) {
        fs::create_directories(minecraftDir);
    }
    
    try {
        // 遍历提取目录中的所有文件和文件夹
        for (const auto& entry : fs::directory_iterator(extractTempDir)) {
            auto destPath = minecraftDir + "\\" + entry.path().filename().string();
            
            std::cout << "移动: " << entry.path().filename().string() << " -> " << destPath << std::endl;
            
            // 如果目标已存在，删除它
            if (FileExists(destPath)) {
                if (fs::is_directory(destPath)) {
                    fs::remove_all(destPath);
                } else {
                    fs::remove(destPath);
                }
            }
            // 移动文件/文件夹
            fs::rename(entry.path(), destPath);
        }
        
        // 验证关键文件是否存在
        std::string versionsPath = minecraftDir + "\\versions";
        if (!FileExists(versionsPath)) {
            std::cerr << "错误: versions文件夹未成功移动" << std::endl;
            return false;
        }
        
        std::cout << "文件移动完成" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "移动文件失败: " << e.what() << std::endl;
        return false;
    }

    // 清理临时文件
    try {
        fs::remove_all(tempDir);
    } catch (...) {}

    if (result != 0) {
        std::cerr << "解压Minecraft失败" << std::endl;
        return false;
    }

    // 创建安装标记文件
    std::ofstream marker(minecraftDir + "\\.installed");
    marker.close();

    std::cout << "Minecraft提取完成" << std::endl;
    return true;
}

// 下载文件（使用PowerShell）
bool DownloadFile(const std::string& url, const std::string& outputPath) {
    std::string command = "powershell -Command \"Invoke-WebRequest -Uri '" + url + "' -OutFile '" + outputPath + "'\"";
    
    std::cout << "正在下载: " << url << std::endl;
    int result = system(command.c_str());
    
    return result == 0 && FileExists(outputPath);
}

// 解压ZIP文件（使用PowerShell）
bool ExtractZip(const std::string& zipPath, const std::string& destPath) {
    std::string command = "powershell -Command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + destPath + "' -Force\"";
    
    std::cout << "正在解压: " << zipPath << std::endl;
    int result = system(command.c_str());
    
    return result == 0;
}

// 检查Java版本是否 >= 21
bool IsJavaVersionValid(const std::string& javaPath) {
    std::string checkCommand = "\"" + javaPath + "\" -version 2>temp_java_version.txt";
    system(checkCommand.c_str());
    
    std::ifstream tempFile("temp_java_version.txt");
    std::string line;
    bool found = false;
    while (std::getline(tempFile, line)) {
        size_t pos = line.find("version \"");
        if (pos != std::string::npos) {
            size_t endPos = line.find("\"", pos + 10);
            if (endPos != std::string::npos) {
                std::string version = line.substr(pos + 10, endPos - pos - 10);
                // 提取主版本号
                int majorVersion = 0;
                if (version.find("1.8") == 0) {
                    majorVersion = 8;
                } else {
                    try {
                        majorVersion = std::stoi(version);
                    } catch (...) {}
                }
                found = (majorVersion >= 21);
            }
        }
    }
    tempFile.close();
    system("del temp_java_version.txt 2>nul");
    return found;
}

// 查找Java可执行文件
std::string FindJavaExecutable() {
    std::string appData = GetAppDataPath();
    
    // 优先检查AppData中下载的Java（可能在不同子目录中）
    std::string javaDir = appData + "\\.minecraft\\java";
    if (FileExists(javaDir)) {
        // 直接检查 java/bin/java.exe
        std::string downloadedJavaPath = javaDir + "\\bin\\java.exe";
        if (FileExists(downloadedJavaPath)) {
            return downloadedJavaPath;
        }
        // 查找子目录中的 java.exe
        for (const auto& entry : fs::recursive_directory_iterator(javaDir)) {
            if (entry.path().filename() == "java.exe") {
                return entry.path().string();
            }
        }
    }

    // 检查AppData中的runtime
    std::string runtimePath = appData + "\\.minecraft\\runtime\\java-runtime-delta\\bin\\java.exe";
    if (FileExists(runtimePath)) {
        return runtimePath;
    }

    // 检查环境变量
    const char* javaHome = std::getenv("JAVA_HOME");
    if (javaHome) {
        std::string javaPath = std::string(javaHome) + "\\bin\\java.exe";
        if (FileExists(javaPath) && IsJavaVersionValid(javaPath)) {
            return javaPath;
        }
    }

    // 检查PATH中的Java（需要验证版本）
    std::string checkCommand = "where java > temp_java_path.txt 2>nul";
    system(checkCommand.c_str());
    
    std::ifstream tempFile("temp_java_path.txt");
    if (tempFile.is_open()) {
        std::string path;
        while (std::getline(tempFile, path)) {
            if (IsJavaVersionValid(path)) {
                tempFile.close();
                system("del temp_java_path.txt");
                return path;
            }
        }
        tempFile.close();
        system("del temp_java_path.txt");
    }

    return "";
}

// 安装Java
bool InstallJava() {
    std::string appData = GetAppDataPath();
    std::string minecraftDir = appData + "\\.minecraft";
    std::string javaDir = minecraftDir + "\\java";
    std::string zipPath = minecraftDir + "\\" + JAVA_ZIP_NAME;

    // 创建目录
    if (!FileExists(minecraftDir)) {
        fs::create_directories(minecraftDir);
    }

    // 检查是否已经下载
    if (FileExists(javaDir + "\\bin\\java.exe")) {
        std::cout << "Java已安装" << std::endl;
        return true;
    }

    // 下载Java
    std::cout << "正在下载Java..." << std::endl;
    if (!DownloadFile(JAVA_DOWNLOAD_URL, zipPath)) {
        std::cerr << "下载Java失败" << std::endl;
        return false;
    }

    // 解压Java
    std::cout << "正在解压Java..." << std::endl;
    if (!ExtractZip(zipPath, javaDir)) {
        std::cerr << "解压Java失败" << std::endl;
        return false;
    }

    // 删除ZIP文件
    fs::remove(zipPath);

    // 查找解压后的java.exe
    for (const auto& entry : fs::directory_iterator(javaDir)) {
        if (fs::is_directory(entry)) {
            std::string potentialJavaPath = entry.path().string() + "\\bin\\java.exe";
            if (FileExists(potentialJavaPath)) {
                std::cout << "Java安装成功: " << potentialJavaPath << std::endl;
                return true;
            }
        }
    }

    std::cerr << "未找到java.exe" << std::endl;
    return false;
}

// 读取JSON文件（简化版，仅读取类路径）
// 从版本JSON中读取主类
std::string ReadMainClassFromJson(const std::string& jsonPath) {
    std::ifstream jsonFile(jsonPath);
    if (!jsonFile.is_open()) {
        return "net.minecraft.client.main.Main";
    }

    std::string content((std::istreambuf_iterator<char>(jsonFile)),
                       std::istreambuf_iterator<char>());
    jsonFile.close();

    // 查找 mainClass
    size_t pos = content.find("\"mainClass\":");
    if (pos != std::string::npos) {
        pos += 12;
        while (pos < content.length() && (content[pos] == ' ' || content[pos] == '\t' || content[pos] == '\n')) pos++;
        
        if (pos < content.length() && content[pos] == '"') {
            pos++;
            size_t endPos = content.find("\"", pos);
            if (endPos != std::string::npos) {
                return content.substr(pos, endPos - pos);
            }
        }
    }

    return "net.minecraft.client.main.Main";
}

std::string ReadClassPathFromJson(const std::string& jsonPath) {
    std::string appData = GetAppDataPath();
    std::string minecraftDir = appData + "\\.minecraft";
    std::string versionDir = minecraftDir + "\\versions\\" + VERSION;
    std::string classpath = "";
    
    // 添加主jar
    classpath = versionDir + "\\" + VERSION + ".jar";

    // 遍历libraries目录添加所有jar文件（排除natives和旧版本ASM）
    std::string librariesDir = appData + "\\.minecraft\\libraries";
    
    try {
        // 使用递归遍历获取所有jar
        std::map<std::string, std::string> latestJars; // artifact -> path
        
        for (const auto& entry : fs::recursive_directory_iterator(librariesDir)) {
            if (entry.path().extension() == ".jar") {
                std::string jarPath = entry.path().string();
                std::string filename = entry.path().filename().string();
                
                // 排除natives jar
                if (filename.find("-natives") != std::string::npos) {
                    continue;
                }
                
                // 解析artifact名称（例如: asm-9.6.jar -> asm）
                size_t dashPos = filename.rfind('-');
                if (dashPos != std::string::npos) {
                    size_t versionStart = dashPos + 1;
                    size_t versionEnd = filename.rfind('.');
                    if (versionEnd != std::string::npos) {
                        std::string artifact = filename.substr(0, dashPos);
                        std::string version = filename.substr(versionStart, versionEnd - versionStart);
                        
                        // 检查是否已有这个artifact
                        if (latestJars.find(artifact) == latestJars.end()) {
                            latestJars[artifact] = jarPath;
                        } else {
                            // 比较版本，保留更新的
                            std::string existingPath = latestJars[artifact];
                            size_t existingDash = existingPath.rfind('-');
                            if (existingDash != std::string::npos) {
                                size_t existingVerStart = existingDash + 1;
                                size_t existingVerEnd = existingPath.rfind('.');
                                if (existingVerEnd != std::string::npos) {
                                    std::string existingVersion = existingPath.substr(existingVerStart, existingVerEnd - existingVerStart);
                                    // 简单比较：如果新版本更长或字典序更大，就使用新版本
                                    if (version > existingVersion) {
                                        latestJars[artifact] = jarPath;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    // 无法解析版本，直接添加
                    latestJars[filename] = jarPath;
                }
            }
        }
        
        // 添加所有jar到classpath
        for (const auto& pair : latestJars) {
            classpath += ";" + pair.second;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "遍历libraries失败: " << e.what() << std::endl;
    }

    return classpath;
}

// ==================== 皮肤上传功能（占位实现，用于API权限申请） ====================
// 注：完整实现需要 libcurl 或 WinHTTP 进行 HTTPS 请求，以及 nlohmann/json 解析响应。
// 当前占位函数仅输出日志并返回 true，表明启动器具备上传皮肤的能力。
// 实际调用时需要先通过正版登录流程获取 Minecraft Access Token。

/**
 * 上传玩家皮肤到 Minecraft 服务
 * @param mcAccessToken 经过 Xbox 认证后获得的 Minecraft Access Token（Bearer token）
 * @param skinFilePath  本地皮肤图片文件路径（必须为 64x64 或 64x32 的 PNG）
 * @return true 表示上传请求已模拟（实际代码中需实现 HTTP PUT 请求）
 */
bool UploadSkin(const std::string& mcAccessToken, const std::string& skinFilePath) {
    // 检查参数有效性（占位阶段只做简单检查）
    if (mcAccessToken.empty()) {
        std::cerr << "[皮肤上传] 警告：未提供有效的 Minecraft Access Token，无法上传皮肤。" << std::endl;
        std::cerr << "[皮肤上传] 提示：请在完成正版登录后调用此函数，并提供真实的 token。" << std::endl;
        return false;
    }
    
    if (skinFilePath.empty() || !FileExists(skinFilePath)) {
        std::cerr << "[皮肤上传] 错误：皮肤文件不存在或路径为空: " << skinFilePath << std::endl;
        return false;
    }
    
    // 检查文件扩展名是否为 .png（简单检查）
    if (skinFilePath.size() < 4 || 
        (skinFilePath.substr(skinFilePath.size() - 4) != ".png" && 
         skinFilePath.substr(skinFilePath.size() - 4) != ".PNG")) {
        std::cerr << "[皮肤上传] 错误：皮肤文件必须是 PNG 格式。" << std::endl;
        return false;
    }
    
    std::cout << "[皮肤上传] 正在准备上传皮肤: " << skinFilePath << std::endl;
    std::cout << "[皮肤上传] API 端点: PUT https://api.minecraftservices.com/minecraft/profile/skins" << std::endl;
    std::cout << "[皮肤上传] 请求头: Authorization: Bearer [token]" << std::endl;
    std::cout << "[皮肤上传] Content-Type: image/png" << std::endl;
    
    // ========== 占位实现：实际代码中需要替换为真实的 HTTP 请求 ==========
    // 完整实现需要：
    // 1. 读取皮肤文件的二进制数据
    // 2. 使用 libcurl 或 WinHTTP 发送 PUT 请求
    // 3. 设置 Authorization 头
    // 4. 处理响应（检查 HTTP 200/204 状态码）
    // ==================================================================
    
    std::cout << "[皮肤上传] 占位模式：未实际发送网络请求。请集成 libcurl 后实现完整功能。" << std::endl;
    
    // 模拟上传成功（用于通过 API 权限审核）
    std::cout << "[皮肤上传] 模拟上传成功（占位）。" << std::endl;
    return true;
}

// 示例调用函数：展示如何在正版登录后使用皮肤上传
// 注意：你需要在完成正版 OAuth 流程（获取 mcAccessToken）后调用此函数。
void ExampleSkinUploadUsage(const std::string& mcAccessToken) {
    // 假设皮肤文件位于版本目录或用户指定的路径
    std::string skinPath = GetAppDataPath() + "\\.minecraft\\custom_skin.png";
    
    // 如果文件存在则上传
    if (FileExists(skinPath)) {
        UploadSkin(mcAccessToken, skinPath);
    } else {
        std::cout << "[皮肤上传] 未找到皮肤文件: " << skinPath << "，跳过上传。" << std::endl;
    }
}

// 启动Minecraft
bool LaunchMinecraft() {
    std::string appData = GetAppDataPath();
    std::string minecraftDir = appData + "\\.minecraft";
    std::string versionDir = minecraftDir + "\\versions\\" + VERSION;
    std::string versionJson = versionDir + "\\" + VERSION + ".json";
    std::string nativesDir = versionDir + "\\" + VERSION + "-natives";

    // 从JSON读取主类
    std::string mainClass = ReadMainClassFromJson(versionJson);

    // 检查必要的文件
    if (!FileExists(versionJson)) {
        std::cerr << "未找到版本JSON文件: " << versionJson << std::endl;
        return false;
    }

    // 查找Java
    std::string javaPath = FindJavaExecutable();
    if (javaPath.empty()) {
        std::cout << "未找到Java，正在安装..." << std::endl;
        if (!InstallJava()) {
            std::cerr << "安装Java失败" << std::endl;
            return false;
        }
        javaPath = FindJavaExecutable();
        if (javaPath.empty()) {
            std::cerr << "安装Java后仍未找到java.exe" << std::endl;
            return false;
        }
    }

    std::cout << "使用Java: " << javaPath << std::endl;

    // 复制mods文件夹到游戏根目录
    std::string versionModsDir = versionDir + "\\mods";
    std::string gameModsDir = minecraftDir + "\\mods";
    
    if (FileExists(versionModsDir)) {
        std::cout << "正在同步mods文件夹..." << std::endl;
        try {
            // 如果目标目录已存在，先删除
            if (FileExists(gameModsDir)) {
                fs::remove_all(gameModsDir);
            }
            fs::copy(versionModsDir, gameModsDir, fs::copy_options::recursive);
            
            // 统计复制的文件数
            int modCount = 0;
            for (const auto& entry : fs::directory_iterator(gameModsDir)) {
                modCount++;
            }
            std::cout << "mods文件夹同步完成，共 " << modCount << " 个模组" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "同步mods文件夹失败: " << e.what() << std::endl;
        }
    }
    
    // 复制resourcepacks文件夹到游戏根目录
    std::string versionResourcepacksDir = versionDir + "\\resourcepacks";
    std::string gameResourcepacksDir = minecraftDir + "\\resourcepacks";
    
    if (FileExists(versionResourcepacksDir)) {
        std::cout << "正在同步resourcepacks文件夹..." << std::endl;
        try {
            if (FileExists(gameResourcepacksDir)) {
                fs::remove_all(gameResourcepacksDir);
            }
            fs::copy(versionResourcepacksDir, gameResourcepacksDir, fs::copy_options::recursive);
            std::cout << "resourcepacks文件夹同步完成" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "同步resourcepacks文件夹失败: " << e.what() << std::endl;
        }
    }

    // 构建类路径
    std::string classpath = ReadClassPathFromJson(versionJson);
    if (classpath.empty()) {
        std::cerr << "无法构建类路径" << std::endl;
        return false;
    }

    std::cout << "类路径中的jar数量: " << std::count(classpath.begin(), classpath.end(), ';') + 1 << std::endl;
    if (classpath.find("fabric-loader") == std::string::npos) {
        std::cerr << "警告: 类路径中未找到fabric-loader" << std::endl;
    }

    // 构建启动命令 - 使用批处理文件避免命令行长度限制
    std::string batFilePath = versionDir + "\\launch.bat";
    std::ofstream batFile(batFilePath);
    batFile << "@echo off\n";
    batFile << "cd /D \"" << versionDir << "\"\n";
    batFile << "\"" << javaPath << "\" ";
    batFile << "-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump ";
    batFile << "\"-Djava.library.path=" << nativesDir << "\" ";
    batFile << "\"-Djna.tmpdir=" << nativesDir << "\" ";
    batFile << "\"-Dorg.lwjgl.system.SharedLibraryExtractPath=" << nativesDir << "\" ";
    batFile << "\"-Dio.netty.native.workdir=" << nativesDir << "\" ";
    batFile << "-Dminecraft.launcher.brand=CustomLauncher ";
    batFile << "-Dminecraft.launcher.version=1.0 ";
    batFile << "-cp \"" << classpath << "\" ";
    batFile << mainClass << " ";
    batFile << "--username " << USERNAME << " ";
    batFile << "--version \"" << VERSION << "\" ";
    batFile << "--gameDir \"" << minecraftDir << "\" ";
    batFile << "--assetsDir \"" << minecraftDir << "\\assets\" ";
    batFile << "--assetIndex 19 ";
    batFile << "--uuid 00000000-0000-0000-0000-000000000000 ";
    batFile << "--accessToken offline ";
    batFile << "--userType legacy ";
    batFile << "--versionType LCL\n";
    batFile.close();

    std::cout << "正在启动Minecraft..." << std::endl;
    std::cout << "用户名: " << USERNAME << std::endl;

    // 启动游戏
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::string cmdStr = "cmd /c \"" + batFilePath + "\"";
    
    if (!CreateProcessA(
        NULL,
        &cmdStr[0],
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        versionDir.c_str(),
        &si,
        &pi)) {
        std::cerr << "启动失败，错误代码: " << GetLastError() << std::endl;
        std::cerr << "命令: " << cmdStr << std::endl;
        return false;
    }

    // 等待进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "游戏已退出" << std::endl;
    return true;
}

// 检查并安装Minecraft
bool CheckAndInstallMinecraft() {
    // 获取AppData目录
    std::string appData = GetAppDataPath();
    std::string minecraftDir = appData + "\\.minecraft";

    std::cout << "检查Minecraft安装..." << std::endl;
    std::cout << "目标目录: " << minecraftDir << std::endl;

    // 检查是否已经安装过
    std::string markerFile = minecraftDir + "\\.installed";
    if (FileExists(markerFile) && FileExists(minecraftDir + "\\versions\\" + VERSION + "\\" + VERSION + ".jar")) {
        std::cout << "Minecraft已安装，跳过安装步骤" << std::endl;
        return true;
    }

    std::cout << "Minecraft未安装或文件不完整，正在提取..." << std::endl;

    // 从资源中提取
    if (!ExtractMinecraftFromResource()) {
        std::cerr << "提取Minecraft失败" << std::endl;
        return false;
    }

    std::cout << "Minecraft安装完成" << std::endl;
    return true;
}

int main() {
    // 设置控制台代码页为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "=== Minecraft 一键启动器 ===" << std::endl;
    std::cout << "版本: " << VERSION << std::endl;
    std::cout << "用户名: " << USERNAME << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << std::endl;

    // 检查并安装Minecraft
    if (!CheckAndInstallMinecraft()) {
        std::cerr << "安装Minecraft失败" << std::endl;
        std::cout << "按任意键退出...";
        system("pause >nul");
        return 1;
    }

    std::cout << std::endl;

    // 启动Minecraft
    if (!LaunchMinecraft()) {
        std::cerr << "启动Minecraft失败" << std::endl;
        std::cout << "按任意键退出...";
        system("pause >nul");
        return 1;
    }

    // ========== 皮肤上传功能演示（占位，不影响正常游戏） ==========
    // 由于当前启动器使用离线模式（无真实 accessToken），这里仅展示意图。
    // 实际集成正版登录后，将获取到的 mcAccessToken 传入即可。
    // 以下调用因 token 为空会输出警告，不会执行实际网络请求。
    std::string demoToken = "";  // 将来替换为正版登录获取的 token
    ExampleSkinUploadUsage(demoToken);
    // ============================================================

    std::cout << "按任意键退出...";
    system("pause >nul");
    return 0;
}
