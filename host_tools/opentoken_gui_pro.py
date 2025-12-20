import tkinter as tk
from tkinter import messagebox, ttk, simpledialog
import threading
import time
import pyperclip
from opentoken_sdk.opentoken import OpenTokenSDK, OATHClient

# --- Constants & Themes ---
THEME = {
    "bg_dark": "#121212",
    "bg_surface": "#1e1e1e",
    "bg_card": "#2d2d2d",
    "fg_main": "#ffffff",
    "fg_dim": "#aaaaaa",
    "accent": "#007acc",
    "danger": "#cf6679",
    "success": "#03dac6"
}

class PremiumAuthenticator(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("OpenToken Pro Authenticator")
        self.geometry("450x650")
        self.configure(bg=THEME["bg_dark"])

        # State
        self.sdk_client = None
        self.accounts = []
        self.filtered_accounts = []
        self.search_query = tk.StringVar()
        self.search_query.trace_add("write", self.on_search)
        self.running = True

        self._setup_styles()
        self._build_ui()
        self._initialize_sdk()
        
        # Start background loop
        self.refresh_thread = threading.Thread(target=self._refresh_loop, daemon=True)
        self.refresh_thread.start()

    def _setup_styles(self):
        style = ttk.Style()
        style.theme_use('clam')
        style.configure("Pro.TProgressbar", 
                        thickness=4, 
                        troughcolor=THEME["bg_dark"], 
                        background=THEME["accent"],
                        borderwidth=0)
        
    def _build_ui(self):
        # Header Area
        header_frame = tk.Frame(self, bg=THEME["bg_dark"], pady=20)
        header_frame.pack(fill="x")
        
        tk.Label(header_frame, text="OpenToken", font=("Inter", 20, "bold"), 
                 fg=THEME["fg_main"], bg=THEME["bg_dark"]).pack()
        
        # Control Bar (Search + Add)
        control_frame = tk.Frame(self, bg=THEME["bg_dark"], padx=20, pady=10)
        control_frame.pack(fill="x")

        self.search_entry = tk.Entry(control_frame, textvariable=self.search_query,
                                    bg=THEME["bg_surface"], fg=THEME["fg_main"],
                                    insertbackground=THEME["fg_main"], borderwidth=0,
                                    font=("Inter", 10), insertwidth=2)
        self.search_entry.pack(side="left", fill="x", expand=True, padx=(0, 10), ipady=8)
        self.search_entry.insert(0, "Search accounts...")
        self.search_entry.bind("<FocusIn>", lambda e: self.search_entry.delete(0, tk.END) if self.search_query.get() == "Search accounts..." else None)

        add_btn = tk.Button(control_frame, text="+ Add", command=self._show_add_dialog,
                           bg=THEME["accent"], fg="white", borderwidth=0,
                           font=("Inter", 10, "bold"), padx=15, pady=5, cursor="hand2")
        add_btn.pack(side="right")

        # Scrollable List Area
        self.canvas = tk.Canvas(self, bg=THEME["bg_dark"], highlightthickness=0)
        self.scrollbar = ttk.Scrollbar(self, orient="vertical", command=self.canvas.yview)
        self.list_inner_frame = tk.Frame(self.canvas, bg=THEME["bg_dark"])
        
        self.list_inner_frame.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.create_window((0, 0), window=self.list_inner_frame, anchor="nw", width=430)
        self.canvas.configure(yscrollcommand=self.scrollbar.set)

        self.canvas.pack(side="left", fill="both", expand=True, padx=(20, 0))
        self.scrollbar.pack(side="right", fill="y")

        # Footer (Progress)
        self.progress = ttk.Progressbar(self, mode='determinate', style="Pro.TProgressbar")
        self.progress.pack(fill="x", side="bottom")

    def _initialize_sdk(self):
        reader = OpenTokenSDK.get_oath_reader()
        if not reader:
            messagebox.showwarning("Device Not Found", "Please connect your OpenToken hardware.")
            return
        
        self.sdk_client = OATHClient(reader)
        try:
            if not self.sdk_client.connect():
                messagebox.showerror("Error", "Could not initialize OATH applet.")
                self.sdk_client = None
        except Exception as e:
            print(f"SDK Init Error: {e}")
            self.sdk_client = None

    def _refresh_loop(self):
        last_sync = 0
        while self.running:
            now = time.time()
            remaining = 30 - (now % 30)
            self.progress['value'] = (remaining / 30) * 100

            # Sync account list every 30s or on startup
            if now - last_sync > 30 or not self.accounts:
                self._load_accounts()
                last_sync = now
            
            # Dynamic update of codes if visible
            self.after(0, self._render_accounts)
            time.sleep(1)

    def _load_accounts(self):
        if not self.sdk_client: return
        try:
            self.accounts = self.sdk_client.list_accounts()
            self.on_search() # Update filtered list
        except:
            self.accounts = []

    def on_search(self, *args):
        query = self.search_query.get().lower()
        if query == "search accounts...": query = ""
        self.filtered_accounts = [a for a in self.accounts if query in a['name'].lower()]
        self._render_accounts()

    def _render_accounts(self):
        # Partial update logic would be better, but for OATH simple re-render is fine
        for widget in self.list_inner_frame.winfo_children():
            widget.destroy()

        if not self.filtered_accounts:
            msg = "No accounts found" if self.accounts else "Connect device to see accounts"
            tk.Label(self.list_inner_frame, text=msg, bg=THEME["bg_dark"], fg=THEME["fg_dim"]).pack(pady=50)
            return

        for acc in self.filtered_accounts:
            card = tk.Frame(self.list_inner_frame, bg=THEME["bg_card"], padx=15, pady=10)
            card.pack(fill="x", pady=5)

            # Left side: Name and Copy Button
            info_frame = tk.Frame(card, bg=THEME["bg_card"])
            info_frame.pack(side="left", fill="both", expand=True)

            tk.Label(info_frame, text=acc['name'], font=("Inter", 10), 
                     fg=THEME["fg_dim"], bg=THEME["bg_card"], anchor="w").pack(fill="x")
            
            if acc['type'] == 'TOTP':
                code = self.sdk_client.calculate(acc['name']) if self.sdk_client else "------"
                code_lbl = tk.Label(info_frame, text=code, font=("JetBrains Mono", 22, "bold"), 
                                   fg=THEME["fg_main"], bg=THEME["bg_card"], anchor="w", cursor="hand2")
                code_lbl.pack(fill="x")
                code_lbl.bind("<Button-1>", lambda e, c=code: self._copy_to_clipboard(c))
            else:
                tk.Label(info_frame, text="HOTP Account", font=("Inter", 14), 
                         fg=THEME["accent"], bg=THEME["bg_card"], anchor="w").pack(fill="x")

            # Right side: Actions (Delete)
            actions = tk.Frame(card, bg=THEME["bg_card"])
            actions.pack(side="right")
            
            del_btn = tk.Button(actions, text="🗑", command=lambda n=acc['name']: self._delete_account(n),
                               bg=THEME["bg_card"], fg=THEME["danger"], borderwidth=0, 
                               font=("Inter", 12), cursor="hand2", activebackground=THEME["bg_card"])
            del_btn.pack()

    def _copy_to_clipboard(self, code):
        pyperclip.copy(code)
        # Briefly change status or show popup would be nice
        print(f"Copied: {code}")

    def _show_add_dialog(self):
        dialog = tk.Toplevel(self)
        dialog.title("Add Account")
        dialog.geometry("350x300")
        dialog.configure(bg=THEME["bg_surface"])
        dialog.transient(self)
        dialog.grab_set()

        tk.Label(dialog, text="Add New Account", font=("Inter", 14, "bold"), 
                 bg=THEME["bg_surface"], fg=THEME["fg_main"], pady=10).pack()

        tk.Label(dialog, text="Name (e.g. Github:me)", bg=THEME["bg_surface"], fg=THEME["fg_dim"]).pack(pady=(10, 0))
        name_entry = tk.Entry(dialog, bg=THEME["bg_dark"], fg=THEME["fg_main"], borderwidth=0)
        name_entry.pack(pady=5, padx=20, fill="x")

        tk.Label(dialog, text="Secret Key (Base32)", bg=THEME["bg_surface"], fg=THEME["fg_dim"]).pack(pady=(10, 0))
        secret_entry = tk.Entry(dialog, bg=THEME["bg_dark"], fg=THEME["fg_main"], borderwidth=0)
        secret_entry.pack(pady=5, padx=20, fill="x")

        def save():
            name = name_entry.get()
            secret = secret_entry.get()
            if not name or not secret: return
            
            try:
                if self.sdk_client.add_account(name, secret):
                    messagebox.showinfo("Success", f"Account '{name}' added!")
                    self._load_accounts()
                    dialog.destroy()
                else:
                    messagebox.showerror("Error", "Failed to add account.")
            except Exception as e:
                messagebox.showerror("Error", str(e))

        tk.Button(dialog, text="Save to OpenToken", command=save, bg=THEME["accent"], 
                  fg="white", borderwidth=0, pady=10).pack(pady=20, padx=20, fill="x")

    def _delete_account(self, name):
        if messagebox.askyesno("Confirm Delete", f"Remove '{name}' from OpenToken?"):
            try:
                if self.sdk_client.delete_account(name):
                    self._load_accounts()
                else:
                    messagebox.showerror("Error", "Deletion failed.")
            except Exception as e:
                messagebox.showerror("Error", str(e))

if __name__ == "__main__":
    app = PremiumAuthenticator()
    app.protocol("WM_DELETE_WINDOW", lambda: (setattr(app, 'running', False), app.destroy()))
    app.mainloop()
