import os
import struct

def create_resource_rc():
    # 压缩.minecraft文件夹
    import subprocess
    subprocess.run(['powershell', '-Command', 
                   "Compress-Archive -Path '.minecraft\\*' -DestinationPath '.minecraft.zip' -Force"],
                  check=True)
    
    # 读取ZIP文件
    with open('.minecraft.zip', 'rb') as f:
        zip_data = f.read()
    
    # 创建资源脚本
    with open('minecraft.rc', 'w') as f:
        f.write('#include <windows.h>\n')
        f.write('MINECRAFT ZIP ".minecraft.zip"\n')
    
    print("资源文件生成完成: minecraft.rc")
    print(f"ZIP文件大小: {len(zip_data)} 字节")

if __name__ == '__main__':
    create_resource_rc()