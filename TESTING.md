# Testing Guide

This guide explains how to test the CS2 Inventory Changer's authentication system and main flows.

## Authentication Flow Testing

### Test Scenario 1: Sign-Up (First Run)

**Objective**: Verify the sign-up process works correctly

**Prerequisites**:
- Fresh installation or deleted `user_config.txt`
- Valid license key (format: CS2-*)
- Run as Administrator

**Steps**:

1. Run `cs2inventory.exe`
2. Application detects no existing configuration
3. Console displays:
   ```
   [*] First time setup
   [*] Please sign up with your license
   ```

4. Enter test data:
   - License Key: `CS2-TEST-1234-5678`
   - Username: `testuser`
   - Password: `password123`
   - Confirm Password: `password123`

5. Check console for:
   ```
   [+] Account created successfully!
   [*] HWID locked: <your_hwid>
   [+] Authentication successful!
   ```

6. Verify `user_config.txt` created in same directory

**Expected Result**: ✅ Sign-up succeeds, config file saved

### Test Scenario 2: Invalid License Format

**Objective**: Verify license validation works

**Prerequisites**:
- Deleted `user_config.txt` for fresh run
- Run as Administrator

**Steps**:

1. Run `cs2inventory.exe`
2. At license prompt, enter invalid format: `INVALID-KEY`
3. Application should display:
   ```
   [ERROR] Invalid license format!
   [!] License must start with 'CS2-'
   ```

**Expected Result**: ✅ Sign-up rejects invalid license

### Test Scenario 3: Login (Subsequent Run)

**Objective**: Verify login and HWID validation works

**Prerequisites**:
- Completed sign-up (Test Scenario 1)
- Same computer/device
- Run as Administrator

**Steps**:

1. Run `cs2inventory.exe` again
2. Application detects existing config:
   ```
   [*] Welcome back, testuser!
   ```

3. Enter login credentials:
   - Username: `testuser`
   - Password: `password123`

4. Application verifies HWID and displays:
   ```
   [+] HWID verified
   [+] Authentication successful!
   ```

**Expected Result**: ✅ Login succeeds, HWID verified

### Test Scenario 4: Wrong Password

**Objective**: Verify password validation

**Prerequisites**:
- Completed sign-up (Test Scenario 1)
- Run as Administrator

**Steps**:

1. Run `cs2inventory.exe`
2. At login prompt, enter:
   - Username: `testuser`
   - Password: `wrongpassword`

3. Application should display:
   ```
   [ERROR] Invalid username or password!
   ```

4. Application exits

**Expected Result**: ✅ Wrong password rejected

### Test Scenario 5: HWID Mismatch

**Objective**: Verify device locking works (must test on different computer)

**Prerequisites**:
- Completed sign-up on Computer A
- Run on Computer B
- Same `user_config.txt` transferred to Computer B

**Steps**:

1. Copy `user_config.txt` from Computer A to Computer B
2. Run `cs2inventory.exe` on Computer B
3. Enter correct credentials at login
4. Application calculates HWID on Computer B
5. Compares with saved HWID from Computer A
6. Application displays:
   ```
   [ERROR] HWID mismatch!
   [!] This account is locked to a different device.
   [*] Contact support to reset your HWID.
   ```

**Expected Result**: ✅ Different device rejected (device locking works)

---

## Auto-Launch Testing

### Test Scenario 6: Steam Auto-Launch

**Objective**: Verify Steam is launched automatically

**Prerequisites**:
- Steam installed at default location: `C:\Program Files (x86)\Steam\steam.exe`
- Completed login authentication
- Run as Administrator

**Steps**:

1. Close Steam completely (if running)
2. Run `cs2inventory.exe`
3. Complete sign-up or login
4. Observe console output:
   ```
   [*] Starting Steam...
   [+] Steam and CS2 launched
   ```

5. Check Task Manager → Applications
   - `steam.exe` should appear within 3-5 seconds

**Expected Result**: ✅ Steam launches automatically

### Test Scenario 7: CS2 Auto-Launch

**Objective**: Verify Counter-Strike 2 is launched

**Prerequisites**:
- Counter-Strike 2 installed
- Steam must be running
- Completed authentication
- Run as Administrator

**Steps**:

1. Run `cs2inventory.exe`
2. Complete authentication flow
3. Console shows:
   ```
   [*] Starting Steam...
   [+] Steam and CS2 launched
   [*] Waiting for CS2 process...
   ```

4. Wait 10-30 seconds for CS2 to launch
5. Check Task Manager → Processes
   - `cs2.exe` should appear

6. Console displays:
   ```
   [+] CS2 found (PID: <process_id>)
   [*] Injecting Inventory Changer...
   ```

**Expected Result**: ✅ CS2 launches and injection begins

---

## Injection Testing

### Test Scenario 8: Injection Success

**Objective**: Verify injection completes without errors

**Prerequisites**:
- CS2 launched and running
- Completed authentication and launcher sequence
- Run as Administrator

**Steps**:

1. Run through authentication and auto-launch
2. Wait for injection sequence:
   ```
   [*] Hooking DirectX...
   [*] Initializing menu system...
   [*] Setting up keybinds (INS)...
   ```

3. Success popup appears:
   ```
   [+] Injection successful!
   
   ╔════════════════════════════════╗
   │  INVENTORY CHANGER IS ACTIVE!  │
   │                                │
   │  Press INS in-game to open     │
   ║════════════════════════════════╝
   ```

4. MessageBox displays success message
5. Console window closes

**Expected Result**: ✅ Injection completes, no errors

### Test Scenario 9: In-Game Menu (INS Key)

**Objective**: Verify in-game menu appears on INS key

**Prerequisites**:
- Injection successful
- Counter-Strike 2 window active
- Run as Administrator

**Steps**:

1. In CS2, press the "Insert" (INS) key
2. Menu overlay should appear (if DirectX hooking implemented)
3. Navigate menu with arrow keys
4. Press INS again to close menu

**Expected Result**: ✅ Menu toggles on INS key press

---

## Error Scenario Testing

### Test Scenario 10: Missing Steam Installation

**Objective**: Verify graceful error handling

**Prerequisites**:
- Steam not installed or path changed
- Completed authentication prior

**Steps**:

1. Rename/move Steam directory temporarily
2. Run `cs2inventory.exe`
3. Complete authentication
4. At auto-launch stage, application tries to start Steam
5. CreateProcess fails
6. Error message displayed:
   ```
   [!] Failed to start Steam/CS2. Make sure Steam is installed.
   ```

**Expected Result**: ✅ Error handled gracefully with helpful message

### Test Scenario 11: Injection Failure

**Objective**: Verify injection error handling

**Prerequisites**:
- Run without Administrator privileges
- CS2 launched
- Completed authentication

**Steps**:

1. Run `cs2inventory.exe` **without** Administrator privileges
2. Complete authentication
3. When injection is attempted, OpenProcess may fail
4. Message displays:
   ```
   [ERROR] Injection failed!
   [!] Make sure you have administrator rights.
   ```

**Expected Result**: ✅ Injection error handled with admin prompt

---

## Config File Testing

### Test Scenario 12: Config Persistence

**Objective**: Verify config file is saved and loaded correctly

**Prerequisites**:
- Completed sign-up (Test Scenario 1)

**Steps**:

1. Locate `user_config.txt` in same directory as `.exe`
2. Open in text editor
3. Verify contents (format: newline-separated):
   ```
   testuser
   password123
   CS2-TEST-1234-5678
   HWID-12345678-<timestamp>
   ```

4. Close application
5. Run again without modifying config
6. Verify login works
7. Modify the username line in config
8. Run again - should fail login with wrong credentials

**Expected Result**: ✅ Config correctly saved and loaded

### Test Scenario 13: Config Integrity

**Objective**: Verify corrupted config handling

**Prerequisites**:
- Completed sign-up

**Steps**:

1. Delete all lines from `user_config.txt` except first line
2. Run `cs2inventory.exe`
3. Application reads incomplete config
4. `GetSystemHWID()` returns empty on blank lines
5. Login fails or shows error
6. Application prompts for sign-up again

**Expected Result**: ✅ Graceful handling of partial config

---

## Performance Testing

### Test Scenario 14: Startup Time

**Objective**: Measure application startup performance

**Prerequisites**:
- Complete sign-up

**Steps**:

1. Time the application from launch to "Injection successful" message
2. Expected timeline:
   - Launch: 0s
   - Console appears: <1s
   - Auth prompt: <1s
   - Auth processing: 1-2s
   - Steam launch: 1-2s (background)
   - CS2 detection: 5-30s (waiting for process)
   - Injection: 1-2s
   - **Total: ~10-35s** (depends on CS2 load time)

**Expected Result**: ✅ Performance reasonable for background launch

---

## Testing Checklist

- [ ] Sign-up with valid license
- [ ] Sign-up with invalid license (rejected)
- [ ] Login with correct credentials
- [ ] Login with wrong password (rejected)
- [ ] HWID verification on same device (accepted)
- [ ] Steam auto-launches
- [ ] CS2 auto-launches
- [ ] Injection completes successfully
- [ ] In-game menu accessible (if implemented)
- [ ] Config file created and persisted
- [ ] Error messages display correctly
- [ ] No crashes or unhandled exceptions

---

## Known Limitations & Future Work

### Current Implementation:
- ✅ Sign-up/Login system
- ✅ HWID device locking
- ✅ Config persistence
- ✅ Auto Steam/CS2 launch
- ⏳ Injection (placeholder code - needs DirectX hooking)
- ⏳ In-game menu (INS key detection)
- ⏳ Inventory modification logic

### To Test Full Flow:
- DirectX hooking implementation needed
- Menu rendering code needed
- Actual inventory modification logic needed

### For Advanced Testing:
- Monitor memory for injection success
- Verify CS2 process has injected DLL
- Check for anti-cheat detection
- Test on various Windows 10/11 versions
