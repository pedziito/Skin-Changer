# Frequently Asked Questions (FAQ)

## General Questions

### Q: What is CS2 Skin Changer?
A: It's an educational tool that demonstrates client-side memory manipulation by modifying weapon skin appearances in Counter-Strike 2. Changes are local only and not visible to other players.

### Q: Is this a cheat?
A: Yes, it modifies game memory and violates Valve's Terms of Service. However, it provides no competitive advantage as it only changes visuals on your screen.

### Q: Is it legal?
A: Technically legal (no laws against modifying your own computer's memory), but it violates Valve's ToS and Steam Subscriber Agreement.

### Q: Will I get banned?
A: **Yes, almost certainly.** Using any memory manipulation tool with CS2 will likely result in a permanent VAC ban.

## VAC and Bans

### Q: Can I get VAC banned for just downloading this?
A: No, simply downloading the source code or executable will not result in a ban. You must actually run it while CS2 is active.

### Q: How likely is a VAC ban?
A: Very high. VAC is designed to detect memory manipulation. Even if it doesn't detect immediately, bans can be delayed.

### Q: Are VAC bans really permanent?
A: Yes. VAC bans are permanent, irreversible, and affect all VAC-secured games on your Steam account.

### Q: Can I appeal a VAC ban?
A: No. Valve does not remove VAC bans under any circumstances, even if you claim it was for "testing" or "educational purposes."

### Q: Will a VAC ban affect all my games?
A: A VAC ban affects all games that use VAC on your account. It doesn't affect non-VAC games.

### Q: Can I use this on a new account?
A: You could, but that account will also likely be VAC banned. This doesn't make it "safe."

## Technical Questions

### Q: How does it work?
A: It:
1. Finds the CS2 process in memory
2. Locates the client.dll module
3. Reads weapon structure addresses using offsets
4. Writes new paint kit IDs to weapon memory
5. Forces the game to display the new skin (client-side only)

### Q: Why can't other players see my skins?
A: Because skin data is stored on Valve's servers, and skins are validated server-side. This tool only changes local memory, not server data.

### Q: What are offsets?
A: Memory offsets are locations in the game's memory where specific data (like weapon skins) is stored. They change with game updates.

### Q: Why do offsets change?
A: Every time CS2 updates, the game code is recompiled, changing where data is stored in memory.

### Q: What is pattern scanning?
A: A technique to find offsets dynamically by searching for unique byte patterns in memory, making the tool more resistant to updates.

### Q: Does this inject DLLs?
A: No, this tool uses external memory reading/writing only. It doesn't inject any code into the game process.

## Usage Questions

### Q: Can I use this in competitive matchmaking?
A: **Absolutely not.** You will get VAC banned. Only use offline with bots.

### Q: Does it work in offline mode?
A: Yes, it works in practice mode with bots or any offline game mode.

### Q: Why don't I see the skin change?
A: Try:
- Dropping the weapon and picking it up
- Switching weapons
- Restarting the round
- Ensuring you selected the correct weapon

### Q: Can I apply multiple skins at once?
A: The current version only applies to the active weapon. You'd need to apply skins one at a time.

### Q: Do skins persist after restarting CS2?
A: No. Skins are only in memory. When CS2 restarts, they reset.

### Q: Can I create custom skins?
A: No, you can only use skins that exist in the game. You select from existing paint kits.

## Build and Setup

### Q: Do I need Visual Studio?
A: Yes, or another C++ compiler that supports C++17 and Windows development.

### Q: Can I build this on Linux?
A: You can compile the code on Linux with cross-compilation tools, but it only runs on Windows (Windows API dependency).

### Q: Can I build this on Mac?
A: Same as Linux - you can cross-compile, but it won't run on Mac.

### Q: What if CMake fails?
A: Ensure you have:
- CMake 3.15+
- Visual Studio with C++ tools
- Windows SDK
- Correct generator specified

### Q: Why doesn't it compile?
A: Common issues:
- Missing Windows SDK
- Wrong CMake generator
- Missing C++ build tools
- Incorrect include paths

## Configuration

### Q: How do I add more skins?
A: Edit `config/skins.json` and add entries with the skin name and paint kit ID.

### Q: Where do I find paint kit IDs?
A: Community databases, game files, or skin ID websites.

### Q: How do I update offsets?
A: Edit `config/offsets.json` with new offset values found using memory scanning tools.

### Q: What if my offsets.json is wrong?
A: The tool won't work. You'll get errors like "Failed to apply skin" or crashes.

### Q: Can I use multiple offset configurations?
A: Currently no, but you could maintain multiple JSON files and swap them.

## Errors and Troubleshooting

### Q: "Failed to find CS2 process"?
A: CS2 isn't running, or the process name changed. Ensure `cs2.exe` is in Task Manager.

### Q: "Failed to find client.dll"?
A: CS2 hasn't fully loaded. Wait until you're in a game, then click "Refresh Process."

### Q: "Failed to apply skin"?
A: Usually means offsets are outdated. Check for updated offset configurations.

### Q: Application crashes on startup?
A: Ensure config files exist:
- `config/offsets.json`
- `config/skins.json`

### Q: Nothing happens when I click Apply?
A: Check:
- You're connected ("Refresh Process" first)
- You selected a weapon and skin
- Status message for errors
- CS2 is actually running

## Development

### Q: Can I contribute?
A: Yes! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Q: What contributions are welcome?
A: Bug fixes, documentation, code quality improvements, better GUI, etc.

### Q: What contributions are NOT welcome?
A: Anti-cheat bypass techniques, online gameplay features, obfuscation, malicious code.

### Q: Can I fork this project?
A: Yes, it's open source under MIT license. But maintain the educational focus and warnings.

### Q: Can I sell this?
A: Technically yes (MIT license allows it), but ethically questionable and may expose you to liability.

## Safety and Ethics

### Q: Is this tool safe to use?
A: The code itself is safe (no malware), but using it will get you banned. It's "safe" in that it won't harm your computer.

### Q: Why was this created?
A: For educational purposes - to demonstrate:
- Windows API usage
- Memory manipulation
- Pattern scanning
- Game client architecture
- Why client-side security is insufficient

### Q: Should I use this on my main account?
A: **Absolutely not.** Use only on accounts you're willing to lose permanently.

### Q: Is there a safe way to use this?
A: There's no way to avoid VAC detection. Use offline only on a test account.

### Q: What about "undetected" versions?
A: Anyone claiming to have an "undetected" version is lying or it will be detected soon. VAC has delayed bans.

## Alternatives

### Q: Are there other skin changers?
A: Yes, but they all have the same VAC ban risk.

### Q: What about server-side skin plugins?
A: Community servers can have plugins that show skins, but these only work on that specific server.

### Q: Can I just buy skins legitimately?
A: Yes! Steam Market and in-game purchases are the legitimate way to get skins.

### Q: What about skin rental services?
A: Some exist but have their own risks. Always safer to buy legitimately.

## Support

### Q: Where can I get help?
A: Read the documentation:
- [README.md](README.md)
- [BUILD.md](BUILD.md)
- [USAGE.md](USAGE.md)
- [SECURITY.md](SECURITY.md)

### Q: Can I contact the developers?
A: Check the repository for contact information or open a GitHub issue.

### Q: Will you help me if I get banned?
A: No. We explicitly warned you about bans. This is your responsibility.

### Q: Can you make it undetectable?
A: No, and we won't. That defeats the educational purpose.

## Legal

### Q: Can I get in legal trouble?
A: Unlikely for just using it yourself, but you could be sued for distributing it maliciously or face CFAA charges in some jurisdictions.

### Q: What about copyright/trademark?
A: This doesn't contain any Valve code or assets. It's a reverse engineering educational tool.

### Q: Am I breaking any laws?
A: Depends on your jurisdiction. You're definitely violating ToS, which is a contract issue, not criminal.

---

**Still have questions?** Check the documentation or open a GitHub issue (for technical questions only, not ban appeals).

**Remember: Using this tool will result in a VAC ban. It's for education only.**
