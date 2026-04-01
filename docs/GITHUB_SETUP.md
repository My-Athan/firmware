# MyAthan — GitHub Organization & Repository Setup Guide

Complete guide for setting up the `My-Athan` GitHub organization with proper policies, branch rules, Actions CI/CD, and deployment pipelines.

---

## 1. Organization Settings (`My-Athan`)

### 1.1 Member Permissions
Go to: **Organization Settings → Member privileges**
- Default repository permission: **Read**
- Allow forking of private repos: **No**
- Allow members to create repositories: **Admin only**

### 1.2 Third-Party Access (IMPORTANT — fixes Claude/MCP integration)
Go to: **Organization Settings → Third-party access → OAuth application policy**
- Set to: **No restrictions** (or explicitly approve the Claude Code integration)
- This resolves the 403 "Resource not accessible by integration" error for Issues and Contents

### 1.3 Actions Permissions
Go to: **Organization Settings → Actions → General**
- Allow all actions and reusable workflows: **Yes**
- Workflow permissions: **Read and write permissions**
- Allow GitHub Actions to create and approve pull requests: **Yes**

---

## 2. Repository Structure

Create 3 repos under the `My-Athan` organization:

| Repo | Visibility | Description |
|---|---|---|
| `My-Athan/firmware` | Private | ESP32-C3 firmware (PlatformIO, C++) |
| `My-Athan/backend` | Private | Backend API (Fastify, TypeScript, PostgreSQL) |
| `My-Athan/web` | Private | Mobile PWA + Admin Panel (pnpm monorepo, React) |

---

## 3. Branch Protection Rules (per repo)

Go to: **Repo Settings → Branches → Add branch protection rule**

### `main` branch (all repos)
| Setting | Value |
|---|---|
| Branch name pattern | `main` |
| Require pull request before merging | **Yes** |
| Required approvals | **1** |
| Dismiss stale PR approvals | **Yes** |
| Require status checks to pass | **Yes** |
| Required checks | `build` (firmware), `test` + `build` (backend/web) |
| Require branches to be up to date | **Yes** |
| Restrict who can push | **Admins only** |
| Allow force pushes | **No** |
| Allow deletions | **No** |

### `develop` branch (optional, for larger teams)
| Setting | Value |
|---|---|
| Branch name pattern | `develop` |
| Require pull request before merging | **Yes** |
| Required approvals | **1** |
| Require status checks to pass | **Yes** |

---

## 4. GitHub Actions — CI/CD Pipelines

### 4.1 Firmware CI (`.github/workflows/build.yml`)

Triggers: push to `main`, PRs, and version tags.

```yaml
name: Firmware Build

on:
  push:
    branches: [main]
    tags: ['v*']
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Install PlatformIO
        run: pip install platformio

      - name: Build firmware
        run: pio run -e esp32c3

      - name: Run tests
        run: pio test -e esp32c3 --without-uploading
        continue-on-error: true  # Tests may need hardware

      - name: Upload build artifact
        if: startsWith(github.ref, 'refs/tags/v')
        uses: actions/upload-artifact@v4
        with:
          name: firmware-${{ github.ref_name }}
          path: .pio/build/esp32c3/firmware.bin

      - name: Publish release
        if: startsWith(github.ref, 'refs/tags/v')
        run: |
          # Calculate SHA256
          SHA256=$(sha256sum .pio/build/esp32c3/firmware.bin | cut -d' ' -f1)
          SIZE=$(stat -c%s .pio/build/esp32c3/firmware.bin)
          VERSION="${GITHUB_REF_NAME#v}"

          # Upload to backend releases API
          curl -X POST "${{ secrets.BACKEND_URL }}/admin/releases" \
            -H "Authorization: Bearer ${{ secrets.ADMIN_TOKEN }}" \
            -F "file=@.pio/build/esp32c3/firmware.bin" \
            -F "version=$VERSION" \
            -F "sha256=$SHA256" \
            -F "size=$SIZE" \
            -F "platform=esp32c3"

      - name: Create GitHub Release
        if: startsWith(github.ref, 'refs/tags/v')
        uses: softprops/action-gh-release@v2
        with:
          files: .pio/build/esp32c3/firmware.bin
          generate_release_notes: true
```

### 4.2 Backend CI (`.github/workflows/ci.yml`)

```yaml
name: Backend CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  test:
    runs-on: ubuntu-latest
    services:
      postgres:
        image: postgres:16
        env:
          POSTGRES_USER: test
          POSTGRES_PASSWORD: test
          POSTGRES_DB: myathan_test
        ports: ['5432:5432']
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5

    steps:
      - uses: actions/checkout@v4

      - uses: pnpm/action-setup@v4
        with:
          version: 9

      - uses: actions/setup-node@v4
        with:
          node-version: 20
          cache: pnpm

      - run: pnpm install --frozen-lockfile
      - run: pnpm run lint
      - run: pnpm run build
      - run: pnpm run test
        env:
          DATABASE_URL: postgresql://test:test@localhost:5432/myathan_test

  deploy:
    needs: test
    if: github.ref == 'refs/heads/main'
    runs-on: ubuntu-latest
    steps:
      - name: Trigger Coolify deploy
        run: |
          curl -X POST "${{ secrets.COOLIFY_WEBHOOK_URL }}" \
            -H "Authorization: Bearer ${{ secrets.COOLIFY_TOKEN }}"
```

### 4.3 Web CI (`.github/workflows/ci.yml`)

```yaml
name: Web CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        package: [shared, app, admin]

    steps:
      - uses: actions/checkout@v4

      - uses: pnpm/action-setup@v4
        with:
          version: 9

      - uses: actions/setup-node@v4
        with:
          node-version: 20
          cache: pnpm

      - run: pnpm install --frozen-lockfile
      - run: pnpm --filter ${{ matrix.package }} run lint
      - run: pnpm --filter ${{ matrix.package }} run build

  deploy-app:
    needs: build
    if: github.ref == 'refs/heads/main'
    runs-on: ubuntu-latest
    steps:
      - name: Trigger Coolify deploy (PWA)
        run: |
          curl -X POST "${{ secrets.COOLIFY_APP_WEBHOOK_URL }}" \
            -H "Authorization: Bearer ${{ secrets.COOLIFY_TOKEN }}"

  deploy-admin:
    needs: build
    if: github.ref == 'refs/heads/main'
    runs-on: ubuntu-latest
    steps:
      - name: Trigger Coolify deploy (Admin)
        run: |
          curl -X POST "${{ secrets.COOLIFY_ADMIN_WEBHOOK_URL }}" \
            -H "Authorization: Bearer ${{ secrets.COOLIFY_TOKEN }}"
```

---

## 5. GitHub Secrets (per repo)

Go to: **Repo Settings → Secrets and variables → Actions**

### Firmware repo secrets
| Secret | Value |
|---|---|
| `BACKEND_URL` | `https://api.myathan.com` |
| `ADMIN_TOKEN` | JWT admin token for release uploads |

### Backend repo secrets
| Secret | Value |
|---|---|
| `COOLIFY_WEBHOOK_URL` | Coolify webhook URL for backend service |
| `COOLIFY_TOKEN` | Coolify API token |
| `DATABASE_URL` | Production PostgreSQL connection string |

### Web repo secrets
| Secret | Value |
|---|---|
| `COOLIFY_APP_WEBHOOK_URL` | Coolify webhook URL for PWA service |
| `COOLIFY_ADMIN_WEBHOOK_URL` | Coolify webhook URL for admin service |
| `COOLIFY_TOKEN` | Coolify API token |

---

## 6. GitHub Labels (all repos)

Create consistent labels across all repos:

| Label | Color | Description |
|---|---|---|
| `epic` | `#7057ff` | Epic / large feature |
| `story` | `#0075ca` | User story |
| `task` | `#e4e669` | Technical task |
| `bug` | `#d73a4a` | Something isn't working |
| `firmware` | `#006b75` | Firmware (ESP32-C3) |
| `backend` | `#5319e7` | Backend API |
| `mobile-app` | `#1d76db` | Mobile PWA |
| `admin` | `#b60205` | Admin panel |
| `infra` | `#fbca04` | Infrastructure / DevOps |
| `phase-1` | `#c5def5` | Phase 1: Firmware Foundation |
| `phase-1.5` | `#c5def5` | Phase 1.5: Infrastructure |
| `phase-2` | `#c5def5` | Phase 2: Backend + Connectivity |
| `phase-3` | `#c5def5` | Phase 3: OTA System |
| `phase-4` | `#c5def5` | Phase 4: Mobile PWA |
| `phase-5` | `#c5def5` | Phase 5: Admin Panel |
| `phase-6` | `#c5def5` | Phase 6: Polish & v2 |

---

## 7. Deployment Pipeline Flow

```
Developer pushes code
        │
        ▼
GitHub Actions triggered
        │
        ├── Lint + Type check
        ├── Build
        ├── Run tests
        │
        ▼ (on main branch merge)
        │
Coolify webhook triggered
        │
        ├── Pull latest code
        ├── Docker build
        ├── Zero-downtime deploy
        └── Health check
```

### Firmware release flow
```
Developer creates git tag (v1.2.0)
        │
        ▼
GitHub Actions triggered
        │
        ├── PlatformIO build
        ├── Calculate SHA256
        ├── Upload binary to backend /admin/releases
        └── Create GitHub Release with binary attached
        │
        ▼
Devices auto-check at 3AM
        │
        ├── GET /releases/latest
        ├── Compare version
        ├── Download + verify SHA256
        ├── Flash OTA
        └── Reboot → heartbeat confirms success
```

---

## 8. Coolify ↔ GitHub Integration

### Setup steps
1. In Coolify dashboard, go to each service → Settings → Source
2. Connect your GitHub account (OAuth)
3. Select the repository and branch (`main`)
4. Enable **Auto Deploy** → Coolify generates a webhook URL
5. Copy the webhook URL → add as GitHub Actions secret (`COOLIFY_WEBHOOK_URL`)
6. In GitHub Actions, the deploy step calls `curl -X POST $COOLIFY_WEBHOOK_URL`

### Alternative: Coolify native webhook (simpler)
Instead of GitHub Actions triggering Coolify, you can:
1. In Coolify → Service → Settings → enable "Auto Deploy on Push"
2. Coolify registers a GitHub webhook automatically
3. Any push to `main` triggers a rebuild directly — no Actions deploy step needed
4. GitHub Actions only runs lint/test/build; Coolify handles deploy independently

**Recommendation**: Use the native Coolify webhook for simplicity. GitHub Actions handles quality gates (lint, test, build). If all checks pass and PR is merged to `main`, Coolify auto-deploys.

---

## 9. Repository Setup Checklist

### For each repo (`firmware`, `backend`, `web`):
- [ ] Create repo in `My-Athan` org (private)
- [ ] Set default branch to `main`
- [ ] Add branch protection rules for `main`
- [ ] Add `.github/workflows/*.yml` CI files
- [ ] Add GitHub Secrets (Coolify webhooks, tokens)
- [ ] Create labels (epic, story, task, phase-*, component)
- [ ] Enable Issues
- [ ] Enable Discussions (optional)
- [ ] Add CODEOWNERS file (optional)

### Organization-level:
- [ ] Fix third-party app access (allow Claude/MCP integration)
- [ ] Set default member permissions to Read
- [ ] Enable Actions for all repos
- [ ] Create GitHub Project board (optional — or use PROJECT.md in each repo)

---

## 10. Git Workflow

### Branching strategy: GitHub Flow (simple)
- `main` — always deployable, protected
- `feature/*` — feature branches from main
- `fix/*` — bug fix branches from main
- `release/*` — optional, for coordinated releases

### Commit convention
```
type: short description

Types:
  feat:     new feature
  fix:      bug fix
  docs:     documentation
  refactor: code restructure
  test:     add/update tests
  ci:       CI/CD changes
  infra:    infrastructure changes
  chore:    maintenance
```

### PR workflow
1. Create feature branch from `main`
2. Push commits
3. Open PR → GitHub Actions runs lint/test/build
4. Code review (1 approval required)
5. Merge to `main` → Coolify auto-deploys
