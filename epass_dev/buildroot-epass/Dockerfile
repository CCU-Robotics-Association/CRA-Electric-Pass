FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# 国内用户可能需要换源
RUN sed -i 's@//.*archive.ubuntu.com@//mirrors.ustc.edu.cn@g' /etc/apt/sources.list.d/ubuntu.sources && \
    sed -i 's/security.ubuntu.com/mirrors.ustc.edu.cn/g' /etc/apt/sources.list.d/ubuntu.sources && \
    sed -i 's@//.*archive.ubuntu.com@//mirrors.ustc.edu.cn@g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.ustc.edu.cn/g' /etc/apt/sources.list


# 安装软件包
RUN apt-get update && apt-get install -y \
    wget \
    unzip \
    build-essential \
    git \
    bc \
    swig \
    locales \
    libncurses-dev \
    libpython3-dev \
    libssl-dev \
    mtd-utils \
    python3-pip \
    fakeroot \
    sudo \
    vim \
    file \
    cpio \
    rsync \
    cmake \
    curl && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# 生成 locale，避免部分工具乱码或编码问题
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# 创建普通用户，避免 root 构建带来的权限问题
ARG USERNAME=builder
ARG UID=1000
ARG GID=1000

RUN set -eux; \
    # --- group: 复用已存在的 GID，或新建同名组 ---
    existing_group="$(getent group "${GID}" | cut -d: -f1 || true)"; \
    if [ -n "${existing_group}" ]; then \
      group_name="${existing_group}"; \
    else \
      group_name="${USERNAME}"; \
      groupadd -g "${GID}" "${group_name}"; \
    fi; \
    \
    # --- user: 复用已存在的 UID，或创建用户并以该 GID 为主组 ---
    existing_user="$(getent passwd "${UID}" | cut -d: -f1 || true)"; \
    if [ -n "${existing_user}" ]; then \
      user_name="${existing_user}"; \
      echo "UID ${UID} already exists as user ${user_name}; will grant sudo to it"; \
    else \
      user_name="${USERNAME}"; \
      useradd -m -u "${UID}" -g "${GID}" -s /bin/bash "${user_name}"; \
    fi; \
    \
    # --- sudoers的处理---
    echo "${user_name} ALL=(ALL) NOPASSWD:ALL" > "/etc/sudoers.d/99-${user_name}"; \
    chmod 0440 "/etc/sudoers.d/99-${user_name}"; \
    usermod -aG sudo "${user_name}" || true

# 工作目录
WORKDIR /buildroot

# 切换到普通用户
USER ${USERNAME}

CMD ["/bin/bash"]