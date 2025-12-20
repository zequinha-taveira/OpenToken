import tkinter as tk
from tkinter import messagebox, ttk
import threading
import time
from opentoken_sdk.opentoken import OpenTokenSDK, OATHClient

class OpenTokenApp:
    def __init__(self, root):
        self.root = root
        self.root.title("OpenToken Authenticator (Native)")
        self.root.geometry("400x500")
        self.root.configure(bg="#1e1e1e")

        # UI Styling
        style = ttk.Style()
        style.theme_use('clam')
        style.configure("TProgressbar", thickness=5, troughcolor="#333", background="#007acc")

        # Header
        self.header = tk.Label(root, text="OpenToken OATH", font=("Arial", 18, "bold"), fg="#fff", bg="#1e1e1e", pady=20)
        self.header.pack()

        # Account List Frame
        self.list_frame = tk.Frame(root, bg="#1e1e1e")
        self.list_frame.pack(fill="both", expand=True, padx=20)

        # Progress Bar
        self.progress = ttk.Progressbar(root, length=400, mode='determinate', style="TProgressbar")
        self.progress.pack(fill="x", side="bottom")

        # SDK Client
        self.client = None
        self.accounts = []
        self.refresh_thread = None
        self.running = True

        self.initialize_sdk()
        self.start_refresh_loop()

    def initialize_sdk(self):
        reader = OpenTokenSDK.get_oath_reader()
        if not reader:
            messagebox.showerror("Error", "No OpenToken device found!")
            return
        
        self.client = OATHClient(reader)
        if not self.client.connect():
            messagebox.showerror("Error", "Failed to connect to device.")
            self.client = None

    def update_ui(self):
        # Clear existing widgets
        for widget in self.list_frame.winfo_children():
            widget.destroy()

        if not self.client:
            tk.Label(self.list_frame, text="Device Not Connected", fg="red", bg="#1e1e1e").pack()
            return

        try:
            self.accounts = self.client.list_accounts()
            for acc in self.accounts:
                frame = tk.Frame(self.list_frame, bg="#2d2d2d", pady=5, padx=10)
                frame.pack(fill="x", pady=5)

                name_label = tk.Label(frame, text=acc['name'], font=("Arial", 10), fg="#aaa", bg="#2d2d2d", anchor="w")
                name_label.pack(side="left")

                if acc['type'] == 'TOTP':
                    code = self.client.calculate(acc['name'])
                    code_label = tk.Label(frame, text=code, font=("Courier", 16, "bold"), fg="#fff", bg="#2d2d2d", anchor="e")
                    code_label.pack(side="right")
                else:
                    tk.Label(frame, text="[HOTP]", fg="#555", bg="#2d2d2d").pack(side="right")
        except Exception as e:
            print(f"Update error: {e}")

    def start_refresh_loop(self):
        def loop():
            while self.running:
                # Calculate progress (30s window)
                now = time.time()
                remaining = 30 - (now % 30)
                self.progress['value'] = (remaining / 30) * 100
                
                # Refresh codes every 1s or if window rolls over
                if int(now) % 30 == 0 or not self.accounts:
                    self.update_ui()
                
                time.sleep(1)

        self.refresh_thread = threading.Thread(target=loop, daemon=True)
        self.refresh_thread.start()

    def on_closing(self):
        self.running = False
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = OpenTokenApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()
