# Security and Privacy Considerations

## ⚠️ Important Legal Notice

**BLE scanning may be subject to privacy regulations in your jurisdiction.**

Before deploying this scanner:

1. **Understand local laws** regarding wireless monitoring and data collection
2. **Obtain proper authorization** if deploying in public or private spaces
3. **Consider privacy implications** of collecting and storing device information
4. **Implement appropriate security measures** for collected data

This tool is designed for:
- ✅ Research and educational purposes
- ✅ Personal network monitoring (your own devices)
- ✅ Authorized security assessments
- ✅ IoT device inventory management

This tool should NOT be used for:
- ❌ Unauthorized tracking or surveillance
- ❌ Stalking or harassment
- ❌ Collection of personal data without consent
- ❌ Violation of privacy laws (GDPR, CCPA, etc.)

## Privacy Best Practices

### 1. Data Minimization

Collect only what you need:

```cpp
// Example: Disable manufacturer data collection if not needed
// In ScanCallbacks::onResult(), comment out:
// if (advertisedDevice->haveManufacturerData()) { ... }
```

### 2. Data Anonymization

Consider hashing MAC addresses:

```cpp
#include "mbedtls/md.h"

void hashMAC(const uint8_t* mac, char* hash) {
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, mac, 6);
  
  unsigned char output[32];
  mbedtls_md_finish(&ctx, output);
  mbedtls_md_free(&ctx);
  
  // Convert to hex string (first 16 bytes)
  for(int i = 0; i < 8; i++) {
    sprintf(hash + (i*2), "%02x", output[i]);
  }
}
```

### 3. Data Retention

Implement automatic data deletion:

- Server-side: Delete data older than X days
- Consider GDPR "right to be forgotten"
- Don't store data indefinitely without reason

### 4. Secure Transmission

For production deployments:

```cpp
// Use HTTPS instead of HTTP
#include <WiFiClientSecure.h>

WiFiClientSecure client;
client.setInsecure();  // For testing only!
// For production, validate certificates:
// client.setCACert(root_ca);

HTTPClient https;
https.begin(client, "https://your-server.com/api/esp32/ble");
```

### 5. Access Control

Server-side authentication:

```python
# Flask example with API key
from flask import request

API_KEY = "your-secret-key-here"

@app.route('/api/esp32/ble', methods=['POST'])
def receive_ble_data():
    # Check API key
    if request.headers.get('X-API-Key') != API_KEY:
        return jsonify({'error': 'Unauthorized'}), 401
    
    # Process data...
```

ESP32 side:

```cpp
// In uploadDevices()
http.addHeader("X-API-Key", "your-secret-key-here");
```

## Security Recommendations

### 1. WiFi Security

- ✅ Use WPA2/WPA3 networks only
- ✅ Avoid public WiFi for data uploads
- ✅ Consider VPN for sensitive deployments
- ✅ Use strong WiFi passwords

### 2. Configuration Security

Never commit credentials:

```bash
# .gitignore should include:
config.h
*.h.local
secrets.h
```

Use environment variables for server deployment:

```python
import os
API_KEY = os.environ.get('API_KEY', 'default-key')
```

### 3. Firmware Updates

- Keep libraries updated
- Monitor for security advisories
- Test updates before deployment

### 4. Physical Security

- Secure the device physically
- Consider tamper detection
- Use enclosures in public deployments

## GDPR Compliance (EU)

If deploying in the EU:

### 1. Legal Basis

Determine your legal basis for processing:
- Legitimate interest (security monitoring)
- Consent (if tracking individuals)
- Legal obligation

### 2. Data Subject Rights

Implement mechanisms for:
- Right to access (show collected data)
- Right to erasure (delete specific MAC addresses)
- Right to restrict processing
- Right to data portability

### 3. Privacy Notice

Post clear signage if monitoring public spaces:
```
BLE MONITORING IN OPERATION
Device presence detection for [PURPOSE]
Data collected: MAC addresses, signal strength
Retention: [X] days
Contact: [EMAIL] for privacy requests
```

### 4. Data Protection Officer

For large deployments, appoint a DPO and maintain:
- Data processing records
- Privacy impact assessment
- Data breach procedures

## Responsible Disclosure

If you discover a security vulnerability:

1. **Do NOT** create a public GitHub issue
2. **Email** security concerns to [your-email]
3. **Include** details: vulnerability description, steps to reproduce
4. **Allow** reasonable time for response (90 days)

We will:
- Acknowledge receipt within 48 hours
- Provide a timeline for fix
- Credit you in the fix announcement (if desired)

## Known Limitations

### 1. MAC Address Randomization

Many modern devices use randomized MAC addresses:
- Makes tracking individuals difficult (good for privacy!)
- May inflate unique device counts
- Consider implementing MAC address pattern detection

### 2. BLE Privacy Features

iOS and Android use privacy-preserving BLE:
- Rotating identifiers
- Encrypted advertisements
- Limited information exposure

### 3. Data Storage

Current implementation:
- Stores data in RAM (lost on restart)
- Server example uses in-memory storage
- For production: use encrypted databases

## Recommendations by Use Case

### Personal Home Network

```
✅ Monitor your own devices
✅ Use local server only
✅ No special legal requirements
✅ Standard WiFi security sufficient
```

### Small Business

```
⚠️ Consider employee privacy
⚠️ Post privacy notices
⚠️ Implement access controls
⚠️ Define data retention policy
⚠️ Consider local privacy laws
```

### Public Space Deployment

```
❌ High legal complexity
❌ Requires legal review
❌ May need permits/licenses
❌ Mandatory privacy notices
❌ Strict data protection
❌ Consider hiring legal counsel
```

### Research/Academic

```
⚠️ Requires ethics approval (IRB)
⚠️ Informed consent may be needed
⚠️ Follow institutional policies
⚠️ Anonymize data
⚠️ Define clear research purpose
```

## Compliance Checklist

Before deployment:

- [ ] Understand legal requirements in your jurisdiction
- [ ] Define purpose of data collection
- [ ] Implement minimum necessary data collection
- [ ] Secure WiFi and server connections
- [ ] Implement access controls
- [ ] Define data retention policy
- [ ] Post privacy notices (if required)
- [ ] Implement data subject rights (if required)
- [ ] Document your data processing
- [ ] Regular security updates
- [ ] Incident response plan

## Resources

- **GDPR**: https://gdpr.eu/
- **CCPA**: https://oag.ca.gov/privacy/ccpa
- **NIST Cybersecurity Framework**: https://www.nist.gov/cyberframework
- **OWASP IoT Security**: https://owasp.org/www-project-internet-of-things/

## Disclaimer

This document provides general guidance only and does not constitute legal advice. Consult with legal counsel familiar with privacy laws in your jurisdiction before deploying BLE monitoring systems.

The authors and contributors of this project assume no liability for misuse or legal violations arising from the use of this software.
