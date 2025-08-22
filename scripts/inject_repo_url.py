#!/usr/bin/env python3
"""
LiveOps Sentinel - Repository URL Injector

This script extracts the Git repository URL from the current repository
and injects it into the README.md file between <!--REPO_URL_START--> and <!--REPO_URL_END--> markers.

Usage:
    python scripts/inject_repo_url.py
"""

import subprocess
import sys
import os
import re
from pathlib import Path

def get_git_remote_url():
    """Extract the Git remote origin URL."""
    try:
        result = subprocess.run(
            ['git', 'config', '--get', 'remote.origin.url'],
            capture_output=True,
            text=True,
            check=True
        )
        url = result.stdout.strip()
        
        # Convert SSH URL to HTTPS if possible
        if url.startswith('git@github.com:'):
            url = url.replace('git@github.com:', 'https://github.com/')
        elif url.startswith('git@gitlab.com:'):
            url = url.replace('git@gitlab.com:', 'https://gitlab.com/')
        
        return url
    except subprocess.CalledProcessError:
        return None
    except FileNotFoundError:
        print("Error: Git is not installed or not in PATH")
        return None

def update_readme_with_url(repo_url):
    """Update README.md with the repository URL."""
    readme_path = Path("README.md")
    
    if not readme_path.exists():
        print("Error: README.md not found")
        return False
    
    # Read the current README content
    try:
        with open(readme_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading README.md: {e}")
        return False
    
    # Define markers
    start_marker = "<!--REPO_URL_START-->"
    end_marker = "<!--REPO_URL_END-->"
    
    # Check if markers exist
    if start_marker not in content or end_marker not in content:
        print("Warning: Repository URL markers not found in README.md")
        print("Please add the following markers to your README.md:")
        print(f"  {start_marker}")
        print("  https://github.com/YOUR_GH_ID/liveops-sentinel")
        print(f"  {end_marker}")
        return False
    
    # Create the replacement content
    if repo_url:
        replacement_url = repo_url
    else:
        replacement_url = "https://github.com/YOUR_GH_ID/liveops-sentinel"
    
    # Create the replacement pattern
    pattern = f"{re.escape(start_marker)}.*?{re.escape(end_marker)}"
    replacement = f"{start_marker}\n{replacement_url}\n{end_marker}"
    
    # Replace the content
    new_content = re.sub(pattern, replacement, content, flags=re.DOTALL)
    
    # Write the updated content back
    try:
        with open(readme_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        
        print(f"Successfully updated README.md with repository URL: {replacement_url}")
        return True
    except Exception as e:
        print(f"Error writing README.md: {e}")
        return False

def main():
    """Main function."""
    print("LiveOps Sentinel - Repository URL Injector")
    print("=" * 50)
    
    # Get the repository URL
    repo_url = get_git_remote_url()
    
    if repo_url:
        print(f"Found repository URL: {repo_url}")
    else:
        print("No Git repository URL found, using placeholder")
    
    # Update README.md
    success = update_readme_with_url(repo_url)
    
    if success:
        print("Repository URL injection completed successfully")
        return 0
    else:
        print("Repository URL injection failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
