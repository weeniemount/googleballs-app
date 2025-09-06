# Stable releases

### Windows
| Arch | Version | Link |
| :--- | :--- | :--- |
| x86_64 | GTK (tar.gz) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/gtk-app-windows-x64.tar.gz) |
| x86_64 | SDL (Native) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/googleballs-desktop-native.exe)
| x86_64 | Tauri (msi) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop_1.0.0_x64_en-US-tauri.msi) |
| x86_64 | Tauri (exe setup) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop_1.0.0_x64-setup-tauri.exe) |
| x86_64 | Electron (exe setup) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop-electron.exe)

install instructions coming soon :bangbang:

### Linux
| Arch | Version | Link |
| :--- | :--- | :--- |
| x86_64 | GTK (tar.gz)  | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/gtk-app-linux-x64.tar.gz) |
| x86_64 | SDL (Native) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/googleballs-desktop-native)
| x86_64 | Tauri (deb) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop_1.0.0_amd64-tauri.deb)
| x86_64 | Tauri (rpm) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop-1.0.0-1.x86_64-tauri.rpm)
| x86_64 | Tauri (AppImage) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop_1.0.0_amd64-tauri.AppImage) |
| x86_64 | Electron (rpm) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop-electron.rpm) |
| x86_64 | Electron (deb) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop-electron.deb)
| x86_64 | Electron (AppImage) | [Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop-electron.AppImage)

install instructions coming soon :bangbang:

### MacOS
[Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls.Desktop-electron.dmg)
NOTE: The macos version is electron.

install instructions coming soon :bangbang:

### iOS
[Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls-ios.ipa)
```
donate 100$ to me and google balls will be on the app store for a year
- ploszukiwacz 2025
```

install instructions coming soon :bangbang:

## Android
[Download](https://github.com/weeniemount/googleballs-app/releases/latest/download/Google.Balls-signed.apk)

install instructions coming soon :bangbang:

## PSVita
[Download .self](https://github.com/weeniemount/googleballs-app/releases/latest/download/google_balls.self)
[Download .vpk](https://github.com/weeniemount/googleballs-app/releases/latest/download/google_balls.vpk)

idk your on your own gang

## 3DS
[Download .3dsx](https://github.com/weeniemount/googleballs-app/releases/latest/download/google-balls-3ds.3dsx)

idk your on your own gang

# Nightly (actions)
Click that actions tab above, pick the latest build and download the artifacts you need

# Tizen TV:
Install Tizen Studio (And packages 8.0 Tizen (9.0 too if you want) and extension Samsung Certificate Extension & TV Extensions-9.0  
Import source code of Tizen version into Tizen Studio as a project (web project, not c/c++ project)
Create a certificate in Certificate Manager, and fill in the information. The onlything that matters is Version when you select the Distributor Certificate. Pick the one with "(Old)" if your TV is older than approximately 5 years old. Otherwise, do the "(New)" option.
Go to the directory of Tizen Studio (C:\tizen-studio\ or /Users/googleballsidkwhaturusernameis/tizen-studio
Go to the tools folder
Open a command line there
On your TV: 
## REMOTE WITH NUMBERS NEEDED
Go to the APPS section.
Go to the settings icon of the app store.
Press 1 2 3 on the remote.
After that, press 1 2 3 4 5.
A developer options menu will appear. 
Now, enable the developer options.
Enter in Local IP of your computer where it prompts you to.
Afer this, go to the Settings.
Go to Connection->Network->Network Status->IP Settings
You should see the local IP of the TV there in the IP adress field. Now remember that.

Back on your PC:
Now in that command line, enter either ./sdb connect (ip) or just sdb connect (ip)
After this, you will see the device appear on your emulators dropdown.
<img width="307" height="102" alt="Screenshot 2025-09-06 at 9 09 06 PM" src="https://github.com/user-attachments/assets/4520b86f-b230-4bf6-9158-2281462065cc" />
Select the TV with the IP you entered in to connect.
Then, click the play button
If you encounter an error within 58%, you are on your own here. I have no idea how to fix this, except if you did 8.0 (New) on an old tizen version TV, then make a new certificate using 7.0 (Old) as the version when selecting Distributor Certificate
Otherwise, you're good to go.
