@echo off
echo ========================================
echo Remove .bat files from Git tracking
echo ========================================
echo.

echo [1/3] Removing .bat files from Git tracking...
git rm --cached fix_release.bat
git rm --cached fix_workflow.bat
git rm --cached fix_workflow_final.bat
git rm --cached fix_vcpkg_final.bat
git rm --cached fix_cmake_final.bat
git rm --cached create_release_now.bat
git rm --cached remove_bat_files.bat

echo.
echo [2/3] Committing the removal...
git commit -m "Remove .bat files from Git tracking - add to .gitignore"

echo.
echo [3/3] Pushing changes...
git push origin main

echo.
echo ========================================
echo BATCH FILES REMOVED FROM GIT!
echo ========================================
echo.
echo The .bat files are now:
echo - Removed from Git tracking
echo - Added to .gitignore
echo - Still available locally for use
echo.
echo You can now use the .bat files locally
echo without them being committed to the repository.
echo.
pause

