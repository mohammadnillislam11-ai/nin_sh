# NINMODS — Master Engineering & Architecture Documentation (DOCS)

> **IMPORTANT ARCHITECTURAL DIRECTIVE (dj CONTRACT)**
> Every rule and interface defined in this document is authoritative and permanent. Do not deviate from these standards in future updates or conversations.

---

## 1. System Overview & Component Structure

NINMODS is an enterprise-grade mobile modding & runtime loader system composed of three tightly coupled modules:
1. **Mobile Loader Client (`DriverLoader`)**: Modern Jetpack Compose Android client (Material 3). Manages background execution, license synchronization, server asset downloads, and root memory interfaces.
2. **External Mod Engine (`refer/nin`)**: Native C++ ARM64 client engine compiled with Android NDK. Runs filelessly in RAM and exposes an interactive stdin authentication gate.
3. **Backend API Server (`app_backend`)**: Node.js / Express / MySQL server providing license validation (`/api/v2/app/activate`), user sessions, and authenticated/static file downloads (`/downloads/*`).

---

## 2. Mod Portfolio Policy & Dashboard Presentation

- **Kernel Mods:** Currently **0 (NONE)**. Do NOT display fake, placeholder, or dummy kernel mod cards on the Dashboard.
- **External Mods:** Strictly **ONE official external mod** built from `refer/nin`.
- **Mod Icon Standard:** The external mod card on the dashboard must display the **Official Software Logo** (`R.drawable.logo_n`). Do NOT replace the official logo with a download button or download icon.
- **Status Indicator:**
  - `AVAILABLE`: Mod is not downloaded on device.
  - `READY`: Mod binary is downloaded and ready to launch.
  - `ACTIVE`: Mod process is currently running in RAM.

---

## 3. GitHub Actions Build & Binary Packaging Flow

- **Source Location:** `C:\Users\Sajala\Documents\mods\refer\nin`
- **Build Pipeline:** Automated via `.github/workflows/build.yml` on GitHub Actions using Android NDK `r25c`.
- **Output Artifact Format:** A standalone self-extracting shell script named **`nin_external.sh`**.
  - Wraps the ARM64 `gemini-cli` binary inside an executable shell script with Base64 encoding.
  - When executed, decodes the binary directly into RAM (`/dev/shm` or `/dev` or `/data/local/tmp`).
  - Spawns the process into memory and immediately unlinks the temporary disk file (fileless execution).
- **Server Deployment Directory:**
  - File path on server: `downloads/external/nin_external.sh`
  - Download URL: `https://ninmods.smmtor.com/downloads/external/nin_external.sh`

---

## 4. Complete Interactive User Flow Contract

### Step 1: Download Confirmation (When NOT installed)
1. User taps the mod card featuring the official software logo.
2. The app inspects local storage: `context.filesDir/external_mods/nin_external.sh`.
3. If the file does not exist:
   - An **Official Material 3 Download Confirmation Dialog** (`ApatchConfirmDownloadDialog`) appears.
   - Text asks the user if they wish to download the external mod package.
   - Contains **"Cancel"** and **"Download"** buttons.
4. If user taps **"Cancel"**, the dialog dismisses and no network activity occurs.
5. If user taps **"Download"**:
   - The animated download dialog (`ApatchDownloadDialog`) appears with a real-time progress bar.
   - The app streams `downloads/external/nin_external.sh` from the backend server to local storage.
   - Grants executable permissions: `chmod 755 nin_external.sh`.
   - Card status changes to `READY`.

### Step 2: Start Confirmation (When installed)
1. User taps the mod card when `nin_external.sh` exists locally.
2. An **Official Material 3 Start Confirmation Dialog** (`ApatchWarningDialog`) appears.
3. Dialog title: **"Start Mod"**.
4. Dialog text: **"Do you want to start NINMODS External in memory?"**
5. Contains **"Cancel"** and **"Start"** buttons.
6. Tapping **"Cancel"** dismisses the dialog.
7. Tapping **"Start"** proceeds to runtime execution.

### Step 3: Background Memory Launch & License Verification
1. Tapping **"Start"** spawns the process in background via root shell using `InteractiveProcessBridge`.
2. The native engine initializes in memory and outputs the authentication prompt on stdout:
   ```
   [?] Enter API Access Key:
   ```
3. `InteractiveProcessBridge` detects this stdout sequence and triggers `onLicensePrompt()`.
4. The app automatically surfaces the **Material 3 License Key Pop-up Dialog** (`ApatchLicenseKeyDialog`):
   - Input field: Pre-filled with `sessionManager.activeLicenseKey` if user has an active plan.
   - Action buttons: **"Cancel"** and **"Verify"**.
5. When the user taps **"Verify"**:
   - The button shows an in-button spinner (`CircularProgressIndicator`) and label `"Verifying..."`.
   - The app pipes the submitted key directly into the running process's stdin: `processBridge.sendInput(key)`.
   - The native binary receives the key and executes an HTTPS request to `https://ninmods.smmtor.com/api/v2/app/activate` with device HWID.
   - **On Success:** The binary outputs `[+] API Key validated successfully. Core engine authorized.`.
     - The bridge detects this message, updates status to `"Active & Running in RAM"`, closes the license dialog, and the mod runs freely in memory.
   - **On Failure:** The binary outputs rejection details, which the dialog renders in red text, allowing the user to re-enter a valid license key.

---

## 5. Live License Clock Policy

1. **Card Visibility:**
   - If the user has **NO active license key** or the remaining time has expired (`remainingSeconds <= 0`), the timer card **MUST NOT BE DISPLAYED AT ALL**.
   - It leaves no empty gaps or error placeholders in the UI.
2. **Real-Time Clock Ticking:**
   - When a plan is active, the clock displays `HH : MM : SS`.
   - Uses monotonic hardware clock tracking (`SystemClock.elapsedRealtime()`) and Jetpack Compose state reading.
   - Recomposes and ticks down live every single second without requiring screen refreshes.

---

## 6. Directory Structure Reference

```
DriverLoader/
├── app/src/main/java/com/andorid/ninmods/
│   ├── config/AppConfig.kt                  # Server base URLs
│   ├── data/InteractiveProcessBridge.kt    # Root stdin/stdout pipeline
│   ├── data/ModDownloader.kt               # Category-aware file downloader
│   ├── data/SessionManager.kt              # Monotonic license time sync
│   ├── ui/clock/LicenseClockCard.kt        # Live ticking countdown card
│   ├── ui/dashboard/DashboardScreen.kt      # Main dashboard container
│   ├── ui/dashboard/external_mods/         # External Mod item & launch bridge
│   └── ui/dialogs/ModActionDialogs.kt      # Download, Start & Verify dialogs
│
app_backend/
├── downloads/
│   ├── external/nin_external.sh            # Target download for external mod
│   └── kernel/                             # Reserved for future kernel mods
│
refer/nin/
├── .github/workflows/build.yml             # Builds & packages nin_external.sh
├── src/main.cpp                            # Entry point & stdin prompt gate
└── src/protect/license.cpp                 # Online HTTP license verification
```
