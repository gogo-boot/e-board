# GitHub Actions CI/CD Guide

This guide explains the automated workflows configured for the MyStation project using GitHub Actions.

## Overview

MyStation uses **two GitHub Actions workflows** for continuous integration and deployment:

1. **Build and Release Firmware** - Compiles ESP32 firmware and creates releases
2. **Deploy Documentation** - Builds and deploys Docusaurus documentation to GitHub Pages

```mermaid
graph TD
    A[Git Push/Tag] --> B{Which Workflow?}
    B -->|Code Changes| C[Build Firmware Workflow]
    B -->|Docs Changes| D[Deploy Docs Workflow]
    B -->|Version Tag| E[Build + Release]

    C --> F[Compile Firmware]
    F --> G[Upload Artifacts]

    D --> H[Build Docusaurus]
    H --> I[Deploy to GitHub Pages]

    E --> F
    E --> J[Create GitHub Release]

    style C fill:#ffe1e1
    style D fill:#e1f5ff
    style E fill:#e1ffe1
```

---

## Workflow 1: Build and Release Firmware

**File**: `.github/workflows/build-firmware.yml`

### Purpose

Automatically builds ESP32 firmware whenever code changes are pushed or a new version is tagged. Creates GitHub Releases
for tagged versions.

### When It Runs

```mermaid
flowchart LR
    A[Trigger Events] --> B{Event Type?}
    B -->|Pull Request to main| C[Build Only]
    B -->|Version Tag v*| D[Build + Release]

    C --> E[Compile Firmware]
    E --> F[Upload Artifacts]

    D --> E
    D --> G[Create GitHub Release]
    G --> H[Attach Binaries]

    style C fill:#fff9e1
    style D fill:#e1ffe1
```

**Trigger Conditions**:

- **Pull Requests** to `main` branch → Build and test
- **Tags** matching `v*` pattern (e.g., `v1.0.0`, `v2.1.3`) → Build and release

**Example**:

```bash
# This triggers build only (PR)
git checkout -b feature/new-display-mode
git commit -m "Add new display mode"
git push origin feature/new-display-mode
# → Open PR → GitHub Actions builds firmware

# This triggers build + release
git tag v1.2.0
git push origin v1.2.0
# → GitHub Actions builds AND creates release
```

### What It Does

#### Job 1: Build

**Environment**: Ubuntu Latest
**Matrix Strategy**: Builds for `esp32-c3-production` environment

**Steps**:

1. **Checkout Code**
   ```yaml
   - uses: actions/checkout@v4
   ```
   Clones the repository to the GitHub Actions runner.

2. **Cache Dependencies**
   ```yaml
   - uses: actions/cache@v4
   ```
   Caches PlatformIO packages and Python pip to speed up builds.
    - Cached items: `~/.cache/pip`, `~/.platformio/.cache`, `~/.platformio/packages`
    - Cache key: OS-specific (e.g., `Linux-pio`)

3. **Setup Python**
   ```yaml
   - uses: actions/setup-python@v5
     with:
       python-version: '3.11'
   ```
   Installs Python 3.11 for PlatformIO.

4. **Install PlatformIO**
   ```bash
   pip install --upgrade platformio==6.1.18
   ```
   Installs specific PlatformIO version for consistency.

5. **Create Secrets Files**
   ```bash
   cat > src/secrets/general_secrets.h << EOF
   #define GOOGLE_API_KEY "${{ secrets.GOOGLE_API_KEY }}"
   #define ENCRYPTION_KEY "${{ secrets.ENCRYPTION_KEY }}"
   #define RMV_API_KEY "${{ secrets.RMV_API_KEY }}"
   EOF
   ```

   **Important**: This step creates the secrets header file from GitHub Secrets.

   **Required Secrets** (configure in GitHub Settings → Secrets):
    - `GOOGLE_API_KEY` - Google Geolocation API key
    - `ENCRYPTION_KEY` - AES encryption key for secure storage
    - `RMV_API_KEY` - RMV public transport API key
    - `ACTION_TOKEN` - GitHub Personal Access Token for releases

6. **Build Firmware**
   ```bash
   pio run -e esp32-c3-production
   ```
   Compiles firmware for ESP32-C3 production environment.

   **Output Files**:
    - `.pio/build/esp32-c3-production/firmware.bin` - Main firmware
    - `.pio/build/esp32-c3-production/bootloader.bin` - Bootloader
    - `.pio/build/esp32-c3-production/partitions.bin` - Partition table
    - `.pio/build/esp32-c3-production/firmware.elf` - Debug symbols

7. **Create Build Info**
   ```bash
   echo "Commit: ${{ github.sha }}" >> build_info.txt
   echo "Branch: ${{ github.ref_name }}" >> build_info.txt
   ```
   Generates metadata file with build information.

8. **Upload Artifacts**
   ```yaml
   - uses: actions/upload-artifact@v4
   ```
   Uploads compiled binaries as workflow artifacts.
    - **Retention**: 30 days
    - **Includes**: All `.bin` files, `firmware.elf`, `build_info.txt`

#### Job 2: Release

**Depends On**: `build` job
**Condition**: Only runs when a version tag is pushed (`if: github.ref_type == 'tag'`)

**Steps**:

1. **Download Artifacts**
   ```yaml
   - uses: actions/download-artifact@v5
   ```
   Downloads all build artifacts from the build job.

2. **Create Release Package**
    - Copies firmware binaries to `release/` directory
    - Creates `FLASHING_INSTRUCTIONS.md` with detailed flashing guide
    - Generates `VERSION.txt` with version information
    - Creates ZIP package: `mystation-firmware-{commit}.zip`

3. **Create GitHub Release**
   ```yaml
   - uses: softprops/action-gh-release@v2
   ```

   Creates a new GitHub Release with:
    - **Name**: `MyStation E-Board v1.2.0`
    - **Tag**: Version tag (e.g., `v1.2.0`)
    - **Files**: All binaries, instructions, and package
    - **Body**: Auto-generated release notes with:
        - What's new
        - Files included
        - Quick flash command
        - Documentation links

### Artifacts Produced

```
mystation-firmware-{sha}/
├── firmware.bin              # Main application binary
├── bootloader.bin            # ESP32 bootloader
├── partitions.bin            # Partition table
├── firmware.elf              # Debug symbols (for crash analysis)
├── littlefs.bin             # Web interface filesystem
├── build_info.txt           # Build metadata
├── config_my_station.html   # Configuration web page
├── FLASHING_INSTRUCTIONS.md # User guide
└── VERSION.txt              # Version information
```

### Configuration

**PlatformIO Environment**: `esp32-c3-production`

Key settings (from `platformio.ini`):

```ini
[env:esp32-c3-production]
platform = espressif32@6.5.0
board = esp32-c3-devkitc-02
framework = arduino
monitor_speed = 115200
build_flags =
    -DBOARD_ESP32_C3
    -DCORE_DEBUG_LEVEL=0  # Production: no debug logs
```

---

## Workflow 2: Deploy Documentation

**File**: `.github/workflows/deploy-docs.yml`

### Purpose

Automatically builds and deploys the Docusaurus documentation website to GitHub Pages whenever documentation files are
updated.

### When It Runs

```mermaid
flowchart TD
    A[Trigger Events] --> B{Event Type?}
    B -->|Push to main| C[Build + Deploy]
    B -->|Push to docs**| D[Build Only]
    B -->|Pull Request| E[Build Only]
    B -->|Manual Trigger| F[Build + Deploy]

    C --> G[Build Docusaurus]
    D --> G
    E --> G
    F --> G

    G --> H{Should Deploy?}
    H -->|main or manual| I[Deploy to GitHub Pages]
    H -->|other branches| J[Upload Artifact Only]

    style C fill:#e1ffe1
    style D fill:#fff9e1
    style E fill:#fff9e1
    style F fill:#e1f5ff
```

**Trigger Conditions**:

- **Push** to `main` branch → Build and deploy
- **Push** to branches starting with `docs` (e.g., `docs-update`, `docs/feature`) → Build only (preview)
- **Pull Request** to `main` → Build only (verify)
- **Manual Trigger** (`workflow_dispatch`) → Build and deploy

**Paths Watched**:

- `docs/**` - Documentation content
- `website/**` - Docusaurus configuration and theme
- `.github/workflows/deploy-docs.yml` - The workflow itself

**Example**:

```bash
# Triggers build + deploy (main branch)
git checkout main
git commit -m "Update user guide"
git push origin main
# → Builds and deploys to GitHub Pages

# Triggers build only (docs branch)
git checkout -b docs-new-feature
git commit -m "Add new feature documentation"
git push origin docs-new-feature
# → Builds but doesn't deploy (preview only)

# Manual trigger (from GitHub UI)
# Go to Actions → Deploy Documentation → Run workflow
# → Builds and deploys regardless of branch
```

### What It Does

#### Job 1: Build

**Environment**: Ubuntu Latest
**Node.js Version**: 22 (Latest LTS)

**Steps**:

1. **Checkout Repository**
   ```yaml
   - uses: actions/checkout@v5
   ```
   Clones the repository with documentation files.

2. **Setup Node.js**
   ```yaml
   - uses: actions/setup-node@v6
     with:
       node-version: '22'
   ```
   Installs Node.js 22 LTS for Docusaurus.

3. **Install Dependencies**
   ```bash
   cd website
   npm install
   ```
   Installs all npm packages from `website/package.json`:
    - `@docusaurus/core`
    - `@docusaurus/preset-classic`
    - Theme packages
    - Plugins (Mermaid, etc.)

4. **Build Docusaurus Site**
   ```bash
   npm run build
   ```

   Runs Docusaurus production build:
    - Processes all markdown files
    - Renders Mermaid diagrams
    - Generates static HTML
    - Optimizes assets (JS, CSS)
    - Creates search index

   **Output**: `website/build/` directory with complete static site

5. **Upload Artifact**
   ```yaml
   - uses: actions/upload-pages-artifact@v3
     with:
       path: ./website/build
   ```
   Uploads the built site for deployment.

#### Job 2: Deploy

**Depends On**: `build` job
**Environment**: `github-pages` (special GitHub environment)
**Condition**: Only deploys from `main` branch or manual trigger

**Permissions Required**:

```yaml
permissions:
  contents: read      # Read repository
  pages: write        # Write to GitHub Pages
  id-token: write     # Verify identity
```

**Steps**:

1. **Deploy to GitHub Pages**
   ```yaml
   - uses: actions/deploy-pages@v4
   ```

   Deploys the built site to GitHub Pages:
    - Publishes to `https://{username}.github.io/{repo}/`
    - Updates the live documentation site
    - Returns the deployment URL

### Concurrency Control

```yaml
concurrency:
  group: "pages"
  cancel-in-progress: false
```

**Purpose**: Prevents multiple deployments from running simultaneously.

- Only one deployment can run at a time
- New deployments wait for current one to finish (no cancellation)
- Ensures stable, consistent deployments

### Deployment URL

After successful deployment, documentation is available at:

```
https://gogo-boot.github.io/e-board/
```

**Structure**:

- `/` - Homepage
- `/docs/user-guide/quick-start` - User documentation
- `/docs/developer-guide/index` - Developer documentation
- `/docs/reference/configuration-keys-quick-reference` - Quick references

---

## Setting Up GitHub Actions

### Required Repository Secrets

Navigate to: **Settings → Secrets and variables → Actions → New repository secret**

| Secret Name      | Description                            | Example                            |
|------------------|----------------------------------------|------------------------------------|
| `GOOGLE_API_KEY` | Google Geolocation API key             | `AIzaSyC...`                       |
| `ENCRYPTION_KEY` | 32-character AES encryption key        | `your-32-char-encryption-key-here` |
| `RMV_API_KEY`    | RMV public transport API key           | `your-rmv-api-key`                 |
| `ACTION_TOKEN`   | GitHub Personal Access Token (classic) | `ghp_...`                          |

**Creating ACTION_TOKEN**:

1. Go to GitHub Settings → Developer settings → Personal access tokens
2. Generate new token (classic)
3. Select scopes: `repo` (full repository access)
4. Copy token and add as secret

### Enabling GitHub Pages

1. Go to **Settings → Pages**
2. **Source**: GitHub Actions
3. No additional configuration needed (workflow handles deployment)

### First-Time Setup Checklist

- [ ] Add all required secrets to repository
- [ ] Enable GitHub Pages with "GitHub Actions" source
- [ ] Push a commit to trigger initial build
- [ ] Verify Actions tab shows green checkmarks
- [ ] Check GitHub Pages URL is accessible

---

## Monitoring and Debugging

### Viewing Workflow Runs

1. Go to **Actions** tab in GitHub repository
2. Click on workflow name (e.g., "Build and Release ESP32 Firmware")
3. Select a specific run to see details
4. Click on job names to see logs

### Common Issues

#### Build Firmware Workflow

**Problem**: Build fails with "secrets not found"

```
Error: GOOGLE_API_KEY not defined
```

**Solution**: Add missing secrets to repository settings.

---

**Problem**: PlatformIO build error

```
Error: Could not find the package with 'espressif32' requirements
```

**Solution**:

- Clear cache: Re-run workflow
- Update PlatformIO version in workflow file

---

**Problem**: Release creation fails

```
Error: Resource not accessible by integration
```

**Solution**:

- Check `ACTION_TOKEN` is valid
- Ensure token has `repo` scope
- Verify token hasn't expired

#### Deploy Docs Workflow

**Problem**: Build succeeds but deploy skipped

```
Deploy job: Skipped
```

**Solution**:

- Check you're on `main` branch
- Or trigger manually via workflow_dispatch

---

**Problem**: npm install fails

```
Error: ECONNRESET
```

**Solution**: Re-run workflow (network issue)

---

**Problem**: Docusaurus build errors

```
Error: Broken link on page /docs/user-guide/index
```

**Solution**:

- Fix broken markdown links
- Check file paths are correct
- Ensure all referenced files exist

### Debugging Tips

**Enable verbose logging**:

```yaml
- name: Build firmware
  run: pio run -e esp32-c3-production -v
  # Add -v flag for verbose output
```

**Add debug steps**:

```yaml
- name: Debug - List files
  run: |
    ls -R .pio/build/
    cat build_info.txt
```

**Download artifacts locally**:

1. Go to workflow run
2. Scroll to "Artifacts" section
3. Click to download
4. Inspect files locally

---

## Workflow Optimization

### Caching Strategy

Both workflows use caching to speed up builds:

**Firmware Workflow**:

```yaml
- uses: actions/cache@v4
  with:
    path: |
      ~/.cache/pip
      ~/.platformio/.cache
      ~/.platformio/packages
    key: ${{ runner.os }}-pio
```

**Benefits**:

- PlatformIO packages cached (~500MB)
- Reduces build time from ~5 minutes to ~2 minutes
- Saves bandwidth

**Docs Workflow**:

```yaml
# npm automatically caches via setup-node@v6
- uses: actions/setup-node@v6
  with:
    node-version: '22'
    cache: 'npm'  # Auto-cache node_modules
```

**Benefits**:

- npm packages cached
- Reduces build time from ~3 minutes to ~1 minute

### Matrix Builds

Firmware workflow uses matrix strategy:

```yaml
strategy:
  matrix:
    environment: [esp32-c3-production]
```

**To add more boards**:

```yaml
strategy:
  matrix:
    environment:
      - esp32-c3-production
      - esp32-s3-production
```

This builds multiple firmware variants in parallel.

### Build Time Comparison

| Workflow       | Without Cache | With Cache |
|----------------|---------------|------------|
| Build Firmware | ~5 min        | ~2 min     |
| Deploy Docs    | ~3 min        | ~1 min     |

---

## Manual Workflow Triggers

### Triggering Documentation Deployment

**Via GitHub UI**:

1. Go to **Actions** tab
2. Select "Deploy Docusaurus Documentation"
3. Click **Run workflow**
4. Select branch (usually `main`)
5. Click **Run workflow** button

**Via GitHub CLI**:

```bash
gh workflow run deploy-docs.yml
```

**Use Case**: Force documentation update without code changes

### Creating a Release

**Via Git Tag**:

```bash
# Create and push tag
git tag v1.0.0
git push origin v1.0.0

# Or create annotated tag
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

**Via GitHub UI**:

1. Go to **Releases** tab
2. Click **Draft a new release**
3. Create new tag (e.g., `v1.0.0`)
4. Click **Publish release**
5. Workflow automatically triggered

---

## Advanced Configuration

### Customizing Build Environments

**Add new PlatformIO environment**:

1. Edit `platformio.ini`:
   ```ini
   [env:esp32-s3-production]
   platform = espressif32@6.5.0
   board = esp32-s3-devkitc-1
   framework = arduino
   build_flags = -DBOARD_ESP32_S3
   ```

2. Update workflow matrix:
   ```yaml
   matrix:
     environment:
       - esp32-c3-production
       - esp32-s3-production
   ```

3. Push changes → Workflow builds both variants

### Conditional Steps

**Example**: Only run tests on PR:

```yaml
- name: Run tests
  if: github.event_name == 'pull_request'
  run: pio test
```

**Example**: Deploy to different environments:

```yaml
- name: Deploy to staging
  if: github.ref == 'refs/heads/develop'
  run: ./deploy-staging.sh

- name: Deploy to production
  if: github.ref == 'refs/heads/main'
  run: ./deploy-production.sh
```

---

## Notifications

### Adding Slack Notifications

Add to workflow:

```yaml
- name: Notify Slack
  if: always()  # Run even if previous steps fail
  uses: slackapi/slack-github-action@v1
  with:
    payload: |
      {
        "text": "Build ${{ job.status }}: ${{ github.ref_name }}"
      }
  env:
    SLACK_WEBHOOK_URL: ${{ secrets.SLACK_WEBHOOK }}
```

### Email Notifications

GitHub automatically sends email on workflow failures if:

- You're watching the repository
- Email notifications are enabled in settings

---

## Security Best Practices

### Secrets Management

✅ **DO**:

- Use repository secrets for sensitive data
- Rotate API keys periodically
- Use environment-specific secrets
- Limit secret access to necessary workflows

❌ **DON'T**:

- Hardcode secrets in workflow files
- Print secrets in logs (`echo ${{ secrets.API_KEY }}`)
- Commit secrets to repository
- Share secrets between unrelated projects

### Permissions

Workflows use principle of least privilege:

```yaml
permissions:
  contents: read    # Only read access by default
  pages: write      # Only when needed for Pages
```

### Dependency Security

- Use specific action versions (`@v4` not `@main`)
- Regularly update action versions
- Review action source code before use
- Use official actions when possible

---

## Cost and Usage Limits

### GitHub Actions Limits

**Free Tier** (Public Repositories):

- ✅ Unlimited minutes
- ✅ Unlimited storage (artifacts expire after 90 days max)

**Private Repositories**:

- 2,000 minutes/month (Free tier)
- 500 MB storage

### Optimization Tips

1. **Use caching** - Reduces build time and bandwidth
2. **Clean up old artifacts** - Set appropriate retention days
3. **Cancel redundant runs** - Push force updates cancel old runs
4. **Use matrix builds** - Parallel execution is free

---

## Troubleshooting Checklist

### Before Opening an Issue

- [ ] Check workflow logs in Actions tab
- [ ] Verify all required secrets are set
- [ ] Confirm branch/tag naming matches triggers
- [ ] Check for typos in workflow YAML
- [ ] Review recent changes to workflow files
- [ ] Test locally with same commands
- [ ] Check GitHub Status (status.github.com)

### Getting Help

1. **Check workflow logs** - Most errors are in the logs
2. **Review this guide** - Common issues documented
3. **Search GitHub Issues** - Similar problems may exist
4. **Ask in Discussions** - Community can help
5. **Open Issue** - Provide logs and reproduction steps

---

## Related Documentation

- [PlatformIO Build System](development-setup.md) - Local build instructions
- [Configuration System](configuration-system.md) - How secrets are used
- [OTA Updates](ota-system.md) - Firmware update mechanism
- [Testing](testing.md) - Running tests locally

---

## Summary

### Build Firmware Workflow

- **Triggers**: Pull requests, version tags
- **Duration**: ~2 minutes (cached)
- **Output**: Compiled binaries + GitHub Release
- **Purpose**: Automated firmware compilation and distribution

### Deploy Docs Workflow

- **Triggers**: Pushes to main/docs branches, manual
- **Duration**: ~1 minute (cached)
- **Output**: Live documentation website
- **Purpose**: Automated documentation deployment

Both workflows ensure:

- ✅ Consistent builds across environments
- ✅ Automated testing and deployment
- ✅ Version tracking and artifacts
- ✅ Reduced manual errors

---

**Questions or issues?** Check the troubleshooting section or open a GitHub issue.

