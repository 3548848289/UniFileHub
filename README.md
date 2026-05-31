# UniFileHub

> 一款集文件管理、个人网盘、在线文档预览、邮件服务、备忘任务、剪贴板历史与绘图工具于一体的数字工作空间平台。

---

# 🇨🇳 中文

## 🚀 项目简介

UniFileHub 是一款多功能一体化文件与效率管理工具，旨在为用户提供统一、高效的数字工作空间。

项目集成了文件管理、个人网盘、在线文档预览、备忘任务、邮件服务、剪贴板历史以及绘图工具等功能，将分散的工作流程整合到一个平台中，帮助用户更高效地管理文件与日常任务。

---

## 🎉 首个正式版本发布

UniFileHub 首个正式版本现已发布。

本版本实现了统一的文件与效率管理平台，集成文件管理、个人网盘、在线预览、备忘任务、邮件服务、剪贴板历史以及绘图功能，支持本地与云端协同使用，提供一体化工作空间体验。

### 📦 发布包

#### UniFileHubServer.tar.gz

* UniFileHub 后端服务 Docker 离线镜像包
* 适用于无云服务器或离线环境部署
* 可通过 Docker 导入后直接运行

```bash
docker load -i UniFileHubServer.tar.gz
```

#### Windows

* `.exe` 客户端安装包

#### Linux

* `.deb` 安装包（Debian / Ubuntu）

---

## ✨ 功能概览

### 📁 文件系统

支持多种文件格式的查看与处理。

#### 文本类文件

* TXT、INI 等纯文本文件
* 文本编码切换
* 临时文本摘录
* Ctrl + F 搜索
* HTML 预览
* Markdown 预览
* 代码语法高亮

#### 表格类文件

* CSV 文件编辑
* 行列增删
* Ctrl + F 搜索
* XLSX 文件查看

#### 媒体文件

* PNG、JPG、SVG 等图片预览
* 图片水印添加
* 音频播放
* 视频播放

#### 专业文件

* PSD 文件预览
* AI 文件预览
* PDF 文件查看
* XMind 缩略图预览

---

### 📂 文件资源管理器

#### 文件树管理

* 树状目录浏览
* 文件快速定位
* 文件标签关联

#### 文件操作

* 文件备份
* 历史版本查看

#### 多窗口预览

* 1×1 布局
* 1×2 布局
* 2×2 布局

---

### 💾 文件备份

* 自定义备份名称
* 自定义备份路径
* 文件恢复
* 云端备份同步

---

### 🏷️ 备忘标签

* 文件关联标签
* 到期时间设置
* 任务详情记录
* 弹窗提醒
* 邮件提醒

---

### 📧 邮件服务

支持 SMTP 邮件发送。

示例配置：

* SMTP Server：smtp.qq.com
* Port：465
* Username：邮箱账号
* Password：SMTP 授权码

---

### 📋 剪贴板历史

* 自动记录复制内容
* 支持文本、图片、链接、文件路径
* 历史搜索
* 一键复制
* 重要内容置顶

---

### 👤 用户系统

* 用户注册
* 用户登录
* JWT 身份验证
* 用户头像上传
* 登录状态持久化

---

### ☁️ 个人网盘

#### 文件管理

* 上传
* 下载
* 删除
* 重命名
* 移动

#### 文件夹管理

* 创建文件夹
* 路径导航

#### 历史记录

* 上传记录
* 下载记录

#### 其他功能

* 自动处理重名文件

---

## 🛣️ 未来计划

* 剪贴板跨设备同步
* 网盘文件分享功能
* 网盘文件移动优化
* 超大文本文件性能优化
* 更多在线协作能力

---

## 🤝 反馈与支持

如果您遇到问题或有功能建议，欢迎提交 Issue 或 Pull Request。

---

© 2026 UniFileHub Team

---

# 🇺🇸 English

## 🚀 Introduction

UniFileHub is an all-in-one productivity and file management platform designed to provide a unified digital workspace.

It integrates file management, personal cloud storage, document preview, task reminders, email services, clipboard history, and drawing tools into a single application, helping users organize files and daily tasks more efficiently.

---

## 🎉 First Official Release

The first official release of UniFileHub is now available.

This version delivers a unified productivity and file management platform, combining file management, personal cloud storage, online preview, task reminders, email services, clipboard history, and drawing tools, supporting both local and cloud-based workflows.

### 📦 Release Packages

#### UniFileHubServer.tar.gz

* Offline Docker image package for the UniFileHub backend service
* Designed for offline or self-hosted environments
* Can be imported and run directly using Docker

```bash
docker load -i UniFileHubServer.tar.gz
```

#### Windows

* `.exe` client installer

#### Linux

* `.deb` installation package (Debian / Ubuntu)

---

## ✨ Features

### 📁 File System

Supports viewing and processing various file formats.

#### Text Files

* TXT and INI files
* Encoding switching
* Temporary text extraction
* Ctrl + F search
* HTML preview
* Markdown preview
* Syntax highlighting

#### Spreadsheet Files

* CSV editing
* Row and column management
* Search support
* XLSX viewing

#### Media Files

* Image preview
* Watermark tools
* Audio playback
* Video playback

#### Professional Files

* PSD preview
* AI preview
* PDF viewer
* XMind thumbnail preview

---

### 📂 File Explorer

* Tree-based navigation
* File tagging
* Backup management
* Version history
* Multi-pane preview layouts

---

### 💾 Backup System

* Custom backup names
* Custom storage locations
* File recovery
* Cloud synchronization

---

### 🏷️ Memo Tags

* File-level tags
* Due dates
* Detailed notes
* Popup reminders
* Email notifications

---

### 📧 Email Service

SMTP-based email delivery support.

---

### 📋 Clipboard History

* Automatic clipboard tracking
* Text, image, link, and file path support
* Search capability
* Quick reuse
* Pin important records

---

### 👤 User System

* User registration
* Authentication
* JWT authorization
* Avatar upload
* Persistent login sessions

---

### ☁️ Personal Cloud Drive

* Upload and download
* Rename and delete
* Move files
* Folder management
* Upload and download history
* Automatic duplicate filename handling

---

## 🛣️ Roadmap

* Cross-device clipboard synchronization
* Cloud drive file sharing
* Improved file movement support
* Better large-text-file performance
* Enhanced collaboration capabilities

---

## 🤝 Feedback & Support

Issues and Pull Requests are welcome.

---

© 2026 UniFileHub Team
