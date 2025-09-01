const { app, BrowserWindow } = require('electron');
const { platform } = require('os');
const path = require('path');

let mainWindow;

app.whenReady().then(() => {
    let iconPath;
	if (process.platform === 'win32') {
		iconPath = path.join(__dirname, 'icons', 'balls.ico');
	} else if (process.platform === 'linux') {
		iconPath = path.join(__dirname, 'icons', 'balls.png');
	} else {
		iconPath = undefined; // macOS will use the default or can set .icns
	}
    mainWindow = new BrowserWindow({
        width: 800,
        height: 600,
        title: "Google Balls Desktop",
        autoHideMenuBar: true,
        icon: iconPath,
    });
    mainWindow.loadFile(path.join(__dirname, 'balls', 'index.html'));
});