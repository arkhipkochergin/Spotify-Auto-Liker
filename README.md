# Spotify Auto-Liker

A desktop application built with C++ and Qt to automate adding tracks to your Spotify "Liked Songs" library from a list or a text file.

---

## 🛠 Prerequisites: Getting Spotify API Keys

Before using the application, you need to register it on the Spotify Developer Portal to get your own credentials.

1.  Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard).
2.  Log in with your Spotify account.
3.  Click **Create app**.
4.  Enter any name and description for the app.
5.  In the **Redirect URIs** field, add exactly: `http://127.0.0.1:8888/callback`.
6.  Check the **Web API** box.
7.  Click **Create**.
8.  On your app's dashboard, click **Settings** to find your **Client ID** and **Client Secret**.

---

## 🚀 Installation & Setup

1.  Download the latest ZIP archive from the [Releases](https://github.com/arkhipkochergin/Spotify-Auto-Liker/releases) section.
2.  Extract the archive to a folder on your computer.
3.  Open the `config.ini` file with any text editor (e.g., Notepad).
4.  Replace the placeholders with your actual keys:
    ```ini
    [Spotify]
    client_id=your_client_id_here
    client_secret=your_client_secret_here
    ```
5.  Save the file and close the editor.

---

## 📖 How to Use

1.  Run the `SpotifyLikerUI.exe` file.
2.  Click the **Connect to Spotify** button. A browser window will open asking for permission to manage your library.
3.  Choose your input method:
    * **Export from list**: Paste your track list directly into the text area.
    * **Export from .txt**: Select a local text file.
4.  **Important Format**: Every line must follow the pattern: `Artist - Song Name`.
5.  Click **Start Export** to parse the list. The app will show how many tracks were recognized.
6.  Click **Like my tracks** to begin the process. The app will search for each track on Spotify and add it to your library.

---

## ✨ Features

* **Bulk Liked Songs**: Add hundreds of tracks at once.
* **Detailed Logs**: The app tracks successful and failed (not found) searches.
* **Recent Actions**: Access logs of your previous operations. You can download them as `.txt` files.
* **Library Cleanup**: A dedicated feature to **Delete all tracks** from your Liked Songs if you want a fresh start.
* **Safety**: Your API keys are stored locally in `config.ini` and are never shared.

---

## ⚖️ License
This project is licensed under the MIT License.
