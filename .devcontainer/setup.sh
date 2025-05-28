#!/bin/bash  
  
# 更新包管理器  
apt-get update  
  
# 安装项目依赖  
apt-get install -y \  
    build-essential \  
    cmake \  
    libcurl4-openssl-dev \  
    libssl-dev \  
    libboost-all-dev \  
    nlohmann-json3-dev \  
    pkg-config \  
    git  
  
# 创建构建目录  
mkdir -p build  
  
echo "Dev container setup complete!"