const fs = require('fs');
const path = require('path');

exports.default = async function(context) {
    const localeDir = path.join(context.appOutDir, 'locales');
    const resourcesDir = path.join(context.appOutDir, 'resources');
    const mainDir = path.join(context.appOutDir);

    // Ignore the resources folder completely
    if (fs.existsSync(resourcesDir)) {
        console.log('Resources folder ignored');
    }

    // Clean up the locales folder
    fs.readdir(localeDir, function(err, files) {
        if (err) {
            console.error('Error reading locale directory:', err);
            return;
        }

        if (!(files && files.length)) return;

        files.forEach(file => {
            // List of files to keep in the locales folder
            const filesToKeep = ['en-US.pak'];
            
            // If the file is not in the list, delete it
            if (!filesToKeep.includes(file)) {
                const filePath = path.join(localeDir, file);
                try {
                    fs.unlinkSync(filePath);
                    console.log(`Deleted: ${filePath}`);
                } catch (deleteErr) {
                    console.error(`Failed to delete ${filePath}:`, deleteErr);
                }
            }
        });
    });

    // Clean up the main directory
    fs.readdir(mainDir, function(err, files) {
        if (err) {
            console.error('Error reading main directory:', err);
            return;
        }

        if (!(files && files.length)) return;

        files.forEach(file => {
            // List of files to keep in the main directory
            const filesToDelete = ['LICENSE.electron.txt', 'LICENSES.chromium.html']; // why thses 2 filets sso big in size/?
            
            // If the file is not in the list, delete it
            if (filesToDelete.includes(file)) {
                const filePath = path.join(mainDir, file);  // <-- Fixed this to use mainDir, not localeDir
                try {
                    fs.unlinkSync(filePath);
                    console.log(`Deleted: ${filePath}`);
                } catch (deleteErr) {
                    console.error(`Failed to delete ${filePath}:`, deleteErr);
                }
            }
        });
    });
};
