import tkinter as tk
from tkinter import messagebox, ttk
import threading
import time
from opentoken_sdk.opentoken import OpenTokenSDK, OATHClient, CTAP2Client

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
        style.configure("TNotebook", background="#1e1e1e", borderwidth=0)
        style.configure("TNotebook.Tab", background="#333", foreground="#fff", padding=[10, 5])
        style.map("TNotebook.Tab", background=[("selected", "#007acc")])

        # Header
        self.header = tk.Label(root, text="OpenToken", font=("Arial", 18, "bold"), fg="#fff", bg="#1e1e1e", pady=10)
        self.header.pack()

        # Tabs
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill="both", expand=True, padx=10, pady=10)

        # OATH Tab
        self.oath_frame = tk.Frame(self.notebook, bg="#1e1e1e")
        self.notebook.add(self.oath_frame, text="OATH")
        
        self.oath_list = tk.Frame(self.oath_frame, bg="#1e1e1e")
        self.oath_list.pack(fill="both", expand=True)

        # FIDO2 Tab
        self.fido_frame = tk.Frame(self.notebook, bg="#1e1e1e")
        self.notebook.add(self.fido_frame, text="FIDO2")
        
        self.fido_list = tk.Frame(self.fido_frame, bg="#1e1e1e")
        self.fido_list.pack(fill="both", expand=True)

        # Progress Bar
        self.progress = ttk.Progressbar(root, length=400, mode='determinate', style="TProgressbar")
        self.progress.pack(fill="x")

        # Status Bar
        self.status_var = tk.StringVar(value="Status: Disconnected")
        self.status_bar = tk.Label(root, textvariable=self.status_var, bd=1, relief=tk.SUNKEN, anchor=tk.W, bg="#2d2d2d", fg="#aaa")
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)

        # SDK Client
        self.client = None
        self.fido_client = None
        self.accounts = []
        self.fido_creds = []
        self.running = True

        self.initialize_sdk()
        self.start_refresh_loop()

    def initialize_sdk(self):
        devices = OpenTokenSDK.list_devices()
        if not devices:
            self.status_var.set("Status: Disconnected")
            return
        
        dev = devices[0]
        self.status_var.set(f"Status: Connected (Serial: {dev.serial})")
        
        # OATH Client
        reader = OpenTokenSDK.get_oath_reader()
        if reader:
            self.client = OATHClient(reader)
            self.client.connect()
        
        # FIDO2 Client
        self.fido_client = CTAP2Client(dev)
        try:
            self.fido_client.connect()
        except:
            self.fido_client = None

    def update_ui(self):
        # Refresh SDK state
        self.initialize_sdk()

        # Update OATH UI
        for widget in self.oath_list.winfo_children():
            widget.destroy()

        if self.client:
            try:
                self.accounts = self.client.list_accounts()
                for acc in self.accounts:
                    frame = tk.Frame(self.oath_list, bg="#2d2d2d", pady=5, padx=10)
                    frame.pack(fill="x", pady=5)
                    name_label = tk.Label(frame, text=acc['name'], font=("Arial", 10), fg="#aaa", bg="#2d2d2d", anchor="w")
                    name_label.pack(side="left")
                    if acc['type'] == 'TOTP':
                        code = self.client.calculate(acc['name'])
                        tk.Label(frame, text=code, font=("Courier", 16, "bold"), fg="#fff", bg="#2d2d2d").pack(side="right")
                    else:
                        tk.Label(frame, text="[HOTP]", fg="#555", bg="#2d2d2d").pack(side="right")
            except:
                pass
        else:
            tk.Label(self.oath_list, text="OATH not available", fg="#555", bg="#1e1e1e").pack(pady=20)

        # Update FIDO2 UI
        for widget in self.fido_list.winfo_children():
            widget.destroy()

        if self.fido_client:
            try:
                self.fido_creds = self.fido_client.list_fido2_credentials()
                if not self.fido_creds:
                    tk.Label(self.fido_list, text="No Resident Keys found", fg="#555", bg="#1e1e1e").pack(pady=20)
                else:
                    for cred in self.fido_creds:
                        frame = tk.Frame(self.fido_list, bg="#2d2d2d", pady=5, padx=10)
                        frame.pack(fill="x", pady=5)
                        tk.Label(frame, text=cred.get('id', 'Unknown RP'), font=("Arial", 10), fg="#fff", bg="#2d2d2d").pack(side="left")
                        tk.Button(frame, text="Delete", bg="#c00", fg="#fff", borderwidth=0).pack(side="right")
            except:
                pass
        else:
            tk.Label(self.fido_list, text="FIDO2 not available", fg="#555", bg="#1e1e1e").pack(pady=20)

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
