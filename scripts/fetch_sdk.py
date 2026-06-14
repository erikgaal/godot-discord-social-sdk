#!/usr/bin/env python3
"""CI helper: download a zipped Discord Social SDK from a URL and install it.

Cross-platform (no unzip/find dependency). Usage:

    python scripts/fetch_sdk.py <url>

The archive must contain a `discord_social_sdk` folder with include/discordpp.h.
"""
import os
import sys
import tempfile
import urllib.request
import zipfile

import setup_sdk  # same directory; Python puts the script dir on sys.path


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: fetch_sdk.py <url>")
    url = sys.argv[1]
    tmp = tempfile.mkdtemp(prefix="discord_sdk_")
    zip_path = os.path.join(tmp, "sdk.zip")

    print("Downloading Discord Social SDK…")
    urllib.request.urlretrieve(url, zip_path)
    with zipfile.ZipFile(zip_path) as z:
        z.extractall(tmp)

    # Locate the folder that directly contains include/discordpp.h.
    sdk_dir = None
    for root, _dirs, files in os.walk(tmp):
        if "discordpp.h" in files and os.path.basename(root) == "include":
            sdk_dir = os.path.dirname(root)
            break
    if not sdk_dir:
        sys.exit("Could not find include/discordpp.h in the downloaded archive.")

    setup_sdk.install(sdk_dir)


if __name__ == "__main__":
    main()
