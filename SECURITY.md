# Security Policy

## Supported Versions

Only the latest firmware release receives security updates. Devices running older versions should update via OTA.

| Version | Supported |
| ------- | --------- |
| 1.0.x (latest) | :white_check_mark: |
| < 1.0 | :x: |

## Reporting a Vulnerability

**Please do NOT open a public GitHub issue for security vulnerabilities.**

### Preferred: GitHub Security Advisories

Report vulnerabilities privately through [GitHub Security Advisories](https://github.com/My-Athan/firmware/security/advisories/new). This keeps the report confidential until a fix is available.

### Alternative: Email

If you prefer email, contact the maintainer directly at **security@myathan.com**.

### What to Expect

- **Acknowledgment** within 72 hours
- **Triage and initial assessment** within 7 days
- **Fix for critical issues** within 30 days
- **Coordinated disclosure** after 90 days, or when a fix is released (whichever comes first)

You will be credited in the fix unless you prefer to remain anonymous.

## Security Measures

### OTA Firmware Updates

- SHA256 verification of firmware binaries before flashing (via mbedtls)
- Dual-partition OTA scheme with automatic rollback if the new firmware fails to boot
- Config backup and restore across updates
- Maximum 3 consecutive OTA failures before entering recovery mode
- Binary size validation (rejects binaries > 1.5 MB)

### WiFi Credential Storage

- Credentials stored in LittleFS on the ESP32-C3 flash partition
- Credentials can be wiped via long button press (5 seconds) for factory reset

### Network Communication

- HTTPS for all communication with the cloud API
- Device authenticates to the cloud using an HMAC-derived API key

### Local REST API

- HTTP endpoints at `myathan.local` for LAN-based configuration
- **No authentication by design** — intended for trusted home network use only
- See [Known Limitations](#known-limitations) for details and mitigations

### Build Integrity

- Library versions pinned in `platformio.ini`
- CI produces SHA256 checksums for every release binary
- Checksums published in GitHub Release notes

### Config Validation

- All incoming configuration is validated before applying
- Cloud config merges use a whitelist approach — only known fields are accepted

## Scope

The following are in scope for security reports:

- OTA verification bypass (SHA256 check circumvention)
- Remote code execution via any input path (REST API, BLE, config merge)
- WiFi credential extraction beyond physical device access
- Buffer overflows in input parsing (JSON, HTTP, BLE GATT)
- Denial of service that permanently bricks the device (not temporary crashes — the watchdog handles restarts)
- Cloud API communication interception or MITM (if HTTPS is bypassable)
- LittleFS config tampering leading to code execution

## Out of Scope

- **Physical access attacks**: If someone has physical access to the ESP32, they own the device. Flash can be read via JTAG/UART.
- **Local REST API lacking authentication**: This is an intentional design decision (see Known Limitations).
- **WiFi network security**: The device relies on the user's network being secured.
- **DFPlayer Mini serial protocol**: Not a network-accessible attack surface.
- **SD card content manipulation**: Requires physical access.
- **Flooding the local HTTP server**: An embedded device with limited resources — expected limitation.
- **Power analysis or side-channel attacks**

## Known Limitations

### Unauthenticated Local API

The REST API at `myathan.local` has no authentication. Anyone on your local network can read device config, trigger athan playback, or change settings. This is a deliberate trade-off for simplicity in a home LAN environment.

**Mitigation**: Isolate the device on a separate VLAN or IoT network segment if this is a concern for your environment.

### Flash Encryption Not Enabled by Default

The ESP32-C3 supports flash encryption, but it is not enabled in the default build. WiFi credentials are stored in plaintext in LittleFS on the flash chip. Physical extraction of the flash would expose them.

### BLE Provisioning Without Encrypted Pairing

During initial WiFi setup, credentials are transmitted over BLE GATT without encrypted pairing. The provisioning window is short (60 seconds before fallback to captive portal), but a nearby attacker could intercept credentials during this window.

### No Certificate Pinning

HTTPS communication to the cloud API uses standard TLS trust chain validation but does not pin certificates. A compromised CA could issue a fraudulent certificate.

### OTA Download Channel

If the OTA URL provided by the cloud server uses HTTP instead of HTTPS, the firmware download would proceed unencrypted. The SHA256 integrity check still protects against tampering, but the binary content would not be confidential.

## Hardware Security Note

Embedded devices have a fundamentally different security posture than cloud services. Physical access to the ESP32-C3 bypasses most software protections. The security model assumes the device operates on a trusted home network behind a properly secured WiFi router.

## Related

For security issues related to the cloud platform, PWA, or admin dashboard, see the [core security policy](https://github.com/My-Athan/core/blob/main/SECURITY.md).
