# DeepWiki（Docusaurus 站点）

本目录是基于 [Docusaurus](https://docusaurus.io/) 的静态文档站点工程，文档内容来自 `docs/deekwiki/`。

## 安装依赖

```bash
npm ci
```

## 本地预览

```bash
npm run start
```

## 构建静态站点

```bash
npm run build
```

构建产物在 `build/`，可部署到任意静态托管平台。

## 发布到 GitHub Pages（推荐）

仓库已配置 GitHub Actions 工作流自动构建并发布到 Pages：

1. GitHub 仓库 → Settings → Pages
2. Source 选择 `GitHub Actions`
3. 合并/推送到 `main` 后会自动发布

访问地址通常为：`https://lilong555.github.io/Adventure-King/`
