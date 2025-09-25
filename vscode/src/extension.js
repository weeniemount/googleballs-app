// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
const vscode = require('vscode');
const path = require('path');
const fs = require('fs');

// This method is called when your extension is activated
// Your extension is activated the very first time the command is executed

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
	// Register the command
	const disposable = vscode.commands.registerCommand('googleballsTab.open', () => {
		// Create a new Webview Panel
		const panel = vscode.window.createWebviewPanel(
			'googleballsTab', // Identifier for the Webview
			'Google Balls', // Title of the tab
			vscode.ViewColumn.One, // Where to display the tab
			{
				enableScripts: true, // Allow JavaScript execution in the Webview
				retainContextWhenHidden: true
			}
		);
		// Set HTML content for the Webview
		panel.iconPath = vscode.Uri.file(path.join(context.extensionPath, 'src', 'images', 'tab.svg')) // Set the icon
		panel.webview.html = getWebviewContent(context, panel);
	});

	context.subscriptions.push(disposable);
}

function getWebviewContent(context, panel) {
    const htmlFilePath = path.join(context.extensionPath, 'src', 'balls.html');
    let htmlContent = fs.readFileSync(htmlFilePath, 'utf8');
    
    return htmlContent;
}

function deactivate() {}
module.exports = {
    activate
};