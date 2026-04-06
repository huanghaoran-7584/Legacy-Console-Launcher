# [你的启动器名称]

[一句话描述你的启动器，例如：一个轻量级的 Minecraft Java Edition 启动器，支持正版账户登录和游戏管理]

## 项目简介

本项目是一个开源/免费的 Minecraft 启动器，旨在为玩家提供安全、便捷的正版游戏启动体验。启动器遵循 [Minecraft EULA](https://www.minecraft.net/zh-hans/eula) 及 [Minecraft 使用准则](https://aka.ms/mcusageguidelines)。

## 核心功能

- **微软账户登录**：通过 OAuth 2.0 设备授权码流程，安全验证用户身份。
- **游戏所有权验证**：调用 Minecraft 服务 API，确认用户已购买 Minecraft Java Edition。
- **玩家档案获取**：自动获取玩家的 UUID 和游戏内名称，用于启动游戏。
- **游戏启动**：根据用户选择的版本，构造 JVM 参数并启动 Minecraft 客户端。

## 技术实现

- 认证流程遵循 [微软 OAuth 2.0 设备授权流](https://learn.microsoft.com/zh-cn/azure/active-directory/develop/v2-oauth2-device-code)。
- 使用 Xbox Live 认证、XSTS 令牌交换，最终获取 Minecraft Access Token。
- 调用的 API 端点包括：
  - `https://api.minecraftservices.com/authentication/login_with_xbox`
  - `https://api.minecraftservices.com/entitlements/mcstore`
  - `https://api.minecraftservices.com/minecraft/profile`

## 遵守准则声明

- 本项目**不会**修改或破解 Minecraft 客户端。
- 本项目**不会**分发 Minecraft 游戏文件或资源。
- 本项目**不会**存储用户的令牌或凭据，仅用于当前会话的认证。
- 本项目**不会**绕过游戏所有权验证或提供离线盗版功能。
- 项目代码完全公开，接受社区监督。

## 开发与使用

### 环境要求

- C++17 或更高版本
- libcurl（HTTP 请求）
- nlohmann/json（JSON 解析）

### 编译与运行

```bash
git clone https://github.com/你的用户名/你的仓库名.git
cd 你的仓库名
# 根据你的构建系统进行编译，例如：
mkdir build && cd build
cmake ..
make