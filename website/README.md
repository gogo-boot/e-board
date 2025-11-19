# MyStation Documentation Website

This directory contains the Docusaurus-based documentation website for MyStation.

## Prerequisites

- Node.js 20.x (LTS) or later
- npm 10.x or later

## Installation

```bash
npm install
```

## Local Development

```bash
npm start
```

This command starts a local development server and opens up a browser window. Most changes are reflected live without
having to restart the server.

## Build

```bash
npm run build
```

This command generates static content into the `build` directory and can be served using any static contents hosting
service.

## Deployment

The documentation is automatically deployed to GitHub Pages via GitHub Actions when changes are pushed to:

- `main` branch
- Any branch starting with `docs**` (e.g., `docs/feature-x`, `docs-update`)

The workflow uses npm for dependency management with `package-lock.json` for deterministic builds.

You can also manually trigger deployment from the Actions tab in GitHub.

## Project Structure

```
website/
├── docs/              -> Points to ../docs (your markdown files)
├── src/
│   ├── css/          # Custom CSS
│   ├── pages/        # React pages (homepage, etc.)
│   └── components/   # Reusable React components
├── static/           # Static files (images, etc.)
├── docusaurus.config.js  # Docusaurus configuration
├── sidebars.js       # Sidebar structure
└── package.json      # Dependencies
```

## Configuration

### Update GitHub Repository Info

Edit `docusaurus.config.js` and update:

- `url`: Your GitHub Pages URL
- `organizationName`: Your GitHub username/org
- `projectName`: Your repository name
- `editUrl`: Link to edit docs on GitHub

### Customize Sidebar

Edit `sidebars.js` to organize your documentation structure.

## Learn More

- [Docusaurus Documentation](https://docusaurus.io/)
- [Deployment Guide](https://docusaurus.io/docs/deployment)

