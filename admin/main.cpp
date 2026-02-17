#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <windows.h>

/**
 * Simple License Generator for CS2 Skin Changer
 * Admin tool - generate and manage licenses
 */

struct License {
    std::string key;
    std::string createdDate;
    std::string expiryDate;
    std::string username;
    bool isActive;
};

class LicenseManager {
public:
    /**
     * Generate a new license key
     */
    static std::string GenerateLicenseKey() {
        // Simple license format: YEAR-RANDOM-MONTH-RANDOM
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);

        char key[256];
        sprintf_s(key, sizeof(key),
            "CS2-%04d-%08X-%02d-%08X",
            timeinfo->tm_year + 1900,
            rand() % 0xFFFFFFFF,
            timeinfo->tm_mon + 1,
            rand() % 0xFFFFFFFF
        );

        return std::string(key);
    }

    /**
     * Create a new license file
     */
    static bool CreateLicense(const std::string& filename, const std::string& username) {
        try {
            std::string licenseKey = GenerateLicenseKey();
            
            time_t now = time(0);
            struct tm* timeinfo = localtime(&now);
            
            char createdDate[30];
            char expiryDate[30];
            
            strftime(createdDate, sizeof(createdDate), "%Y-%m-%d %H:%M:%S", timeinfo);
            
            // Set expiry to 30 days from now
            time_t expiry = now + (30 * 24 * 3600);
            struct tm* expiry_info = localtime(&expiry);
            strftime(expiryDate, sizeof(expiryDate), "%Y-%m-%d %H:%M:%S", expiry_info);

            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "[ERROR] Failed to create license file: " << filename << "\n";
                return false;
            }

            file << licenseKey << "\n";
            file << "USERNAME=" << username << "\n";
            file << "CREATED=" << createdDate << "\n";
            file << "EXPIRY=" << expiryDate << "\n";
            file << "ACTIVE=true\n";

            file.close();

            std::cout << "[+] License created successfully!\n";
            std::cout << "    File: " << filename << "\n";
            std::cout << "    Key: " << licenseKey << "\n";
            std::cout << "    User: " << username << "\n";
            std::cout << "    Created: " << createdDate << "\n";
            std::cout << "    Expiry: " << expiryDate << "\n";

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception: " << e.what() << "\n";
            return false;
        }
    }

    /**
     * Validate a license file
     */
    static bool ValidateLicense(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "[ERROR] License file not found: " << filename << "\n";
                return false;
            }

            std::string line;
            bool hasKey = false;
            bool isActive = false;

            while (std::getline(file, line)) {
                if (line.find("CS2-") == 0) {
                    hasKey = true;
                    std::cout << "[+] Valid license key format\n";
                }
                if (line.find("ACTIVE=true") != std::string::npos) {
                    isActive = true;
                }
                std::cout << "    " << line << "\n";
            }

            file.close();

            if (hasKey && isActive) {
                std::cout << "[+] License is VALID\n";
                return true;
            }
            else {
                std::cout << "[-] License is INVALID\n";
                return false;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception: " << e.what() << "\n";
            return false;
        }
    }

    /**
     * Revoke a license
     */
    static bool RevokeLicense(const std::string& filename) {
        try {
            std::ifstream infile(filename);
            if (!infile.is_open()) {
                std::cerr << "[ERROR] License file not found: " << filename << "\n";
                return false;
            }

            std::vector<std::string> lines;
            std::string line;

            while (std::getline(infile, line)) {
                if (line.find("ACTIVE=") != std::string::npos) {
                    lines.push_back("ACTIVE=false");
                }
                else {
                    lines.push_back(line);
                }
            }

            infile.close();

            // Write revoked license
            std::ofstream outfile(filename);
            for (const auto& l : lines) {
                outfile << l << "\n";
            }
            outfile.close();

            std::cout << "[+] License revoked successfully\n";
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception: " << e.what() << "\n";
            return false;
        }
    }
};

/**
 * Admin menu
 */
void ShowMenu() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          CS2 SKIN CHANGER - ADMIN LICENSE MANAGER              ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║  1. Generate New License                                      ║\n";
    std::cout << "║  2. Validate License File                                     ║\n";
    std::cout << "║  3. Revoke License                                            ║\n";
    std::cout << "║  4. Exit                                                      ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
}

int main() {
    SetConsoleTitleA("CS2 Skin Changer - Admin License Manager");

    std::cout << "[*] CS2 Skin Changer - Admin Panel v1.0\n";
    std::cout << "[*] License Management Utility\n\n";

    // Admin password protection (simple check)
    std::string password;
    std::cout << "Enter admin password: ";
    std::cin >> password;

    // Simple hardcoded admin check (in production, use proper authentication)
    if (password != "Mao770609") {
        std::cout << "[ERROR] Invalid password!\n";
        return 1;
    }

    std::cout << "[+] Admin access granted\n\n";

    int choice = 0;
    while (true) {
        ShowMenu();
        std::cout << "\nSelect option: ";
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 1) {
            // Generate new license
            std::cout << "\n[*] Generate New License\n";
            std::cout << "Username: ";
            std::string username;
            std::getline(std::cin, username);

            std::cout << "Filename (default: license.key): ";
            std::string filename;
            std::getline(std::cin, filename);
            if (filename.empty()) {
                filename = "license.key";
            }

            LicenseManager::CreateLicense(filename, username);
        }
        else if (choice == 2) {
            // Validate license
            std::cout << "\n[*] Validate License\n";
            std::cout << "Filename: ";
            std::string filename;
            std::getline(std::cin, filename);

            if (LicenseManager::ValidateLicense(filename)) {
                std::cout << "[+] Validation passed\n";
            }
            else {
                std::cout << "[-] Validation failed\n";
            }
        }
        else if (choice == 3) {
            // Revoke license
            std::cout << "\n[*] Revoke License\n";
            std::cout << "Filename: ";
            std::string filename;
            std::getline(std::cin, filename);

            std::cout << "Are you sure? (yes/no): ";
            std::string confirm;
            std::getline(std::cin, confirm);

            if (confirm == "yes") {
                if (LicenseManager::RevokeLicense(filename)) {
                    std::cout << "[+] License revoked\n";
                }
            }
            else {
                std::cout << "[*] Revocation cancelled\n";
            }
        }
        else if (choice == 4) {
            std::cout << "[*] Exiting admin panel\n";
            break;
        }
        else {
            std::cout << "[ERROR] Invalid option\n";
        }
    }

    return 0;
}
