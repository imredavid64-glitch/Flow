import tkinter as tk
from tkinter import scrolledtext, messagebox, filedialog
import time
import math
import random
import re

class FlowInterpreter:
    def __init__(self, output_func):
        self.variables = {}
        self.output_func = output_func
        self.program = []
        
    def sanitize(self, text):
        return text.strip().replace('"', '')

    def is_numeric(self, s):
        try:
            float(s)
            return True
        except ValueError:
            return False

    def run(self, code):
        self.variables = {} # Reset memory for new run
        self.program = [line.strip() for line in code.split('\n') if line.strip()]
        self.execute_range(0, len(self.program))

    def execute_range(self, start, end):
        i = start
        while i < end:
            line = self.program[i]
            
            # Skip comments
            if line.lower().startswith("note:"):
                i += 1
                continue

            # SHOW Command
            if line.lower().startswith("show "):
                content = line[5:].strip()
                if content.lower() == "time":
                    self.output_func(f"> {time.strftime('%H:%M:%S')}")
                elif content in self.variables:
                    self.output_func(f"> {self.variables[content]}")
                else:
                    self.output_func(f"> {self.sanitize(content)}")

            # CREATE / SET Command
            elif "called" in line.lower() or line.lower().startswith("set "):
                parts = re.findall(r'\w+|".*?"', line)
                # Create a Number called X set to 10
                if "called" in line.lower():
                    try:
                        name_idx = parts.index("called") + 1
                        val_idx = len(parts) - 1
                        name = parts[name_idx]
                        val = self.sanitize(parts[val_idx])
                        self.variables[name] = float(val) if self.is_numeric(val) else val
                    except: pass
                # Set X to 10
                elif line.lower().startswith("set "):
                    try:
                        name = parts[1]
                        val = self.sanitize(parts[3])
                        self.variables[name] = float(val) if self.is_numeric(val) else val
                    except: pass

            # MATH Commands
            elif any(op in line.lower() for op in ["add ", "subtract ", "multiply ", "divide "]):
                parts = line.split()
                try:
                    op = parts[0].lower()
                    val = float(parts[1])
                    name = parts[-1]
                    if name in self.variables:
                        if op == "add": self.variables[name] += val
                        elif op == "subtract": self.variables[name] -= val
                        elif op == "multiply": self.variables[name] *= val
                        elif op == "divide": self.variables[name] /= val
                except: pass
                
            i += 1

class FlowPad:
    def __init__(self, root):
        self.root = root
        self.root.title("FlowPad Professional IDE - Untitled.flow")
        self.root.geometry("1100x800")
        self.interpreter = FlowInterpreter(self.write_to_shell)

        # High-Contrast IDE Theme
        self.bg_dark = "#1e1e1e"
        self.bg_editor = "#252526"
        self.bg_shell = "#1e1e1e"
        self.fg_text = "#d4d4d4"
        self.accent = "#007acc"
        self.highlight_kw = "#569cd6"
        self.highlight_str = "#ce9178"
        self.highlight_com = "#6a9955"
        self.highlight_num = "#b5cea8"

        self.root.configure(bg=self.bg_dark)

        # Main Splitter
        self.paned_window = tk.PanedWindow(root, orient=tk.VERTICAL, bg="#333", sashwidth=4)
        self.paned_window.pack(fill=tk.BOTH, expand=1)

        # Editor Area with Line Numbers
        self.editor_frame = tk.Frame(self.paned_window, bg=self.bg_editor)
        
        self.editor_header = tk.Frame(self.editor_frame, bg="#2d2d2d", height=30)
        self.editor_header.pack(fill=tk.X)
        tk.Label(self.editor_header, text="  FLOW SCRIPT EDITOR", bg="#2d2d2d", fg="#aaa", font=("Segoe UI", 8, "bold")).pack(side=tk.LEFT)

        self.text_container = tk.Frame(self.editor_frame, bg=self.bg_editor)
        self.text_container.pack(fill=tk.BOTH, expand=1)

        self.line_nums = tk.Text(self.text_container, width=4, padx=5, takefocus=0, border=0,
                                 background="#333", foreground="#858585", state='disabled',
                                 font=("Courier New", 12))
        self.line_nums.pack(side=tk.LEFT, fill=tk.Y)

        self.editor = scrolledtext.ScrolledText(self.text_container, font=("Courier New", 12), undo=True, 
                                                bg=self.bg_editor, fg=self.fg_text, 
                                                insertbackground="white", borderwidth=0, highlightthickness=0)
        self.editor.pack(side=tk.LEFT, fill=tk.BOTH, expand=1)
        
        self.paned_window.add(self.editor_frame, height=500)

        # Shell Area
        self.shell_frame = tk.Frame(self.paned_window, bg=self.bg_shell)
        
        self.shell_header = tk.Frame(self.shell_frame, bg="#2d2d2d", height=30)
        self.shell_header.pack(fill=tk.X)
        tk.Label(self.shell_header, text="  FLOW DEBUG SHELL", bg="#2d2d2d", fg="#aaa", font=("Segoe UI", 8, "bold")).pack(side=tk.LEFT)

        self.shell = scrolledtext.ScrolledText(self.shell_frame, font=("Courier New", 12), state='disabled', 
                                                bg=self.bg_shell, fg="#4ec9b0", borderwidth=0, highlightthickness=0)
        self.shell.pack(fill=tk.BOTH, expand=1)
        self.paned_window.add(self.shell_frame, height=200)

        # Menu Bar
        self.setup_menus()
        
        # Events
        self.editor.bind("<KeyRelease>", self.on_content_changed)
        self.editor.bind("<Shift-Tab>", self.outdent)
        root.bind("<F5>", lambda e: self.run_module())
        
        # Syntax Tags
        self.editor.tag_configure("keyword", foreground=self.highlight_kw)
        self.editor.tag_configure("string", foreground=self.highlight_str)
        self.editor.tag_configure("comment", foreground=self.highlight_com)
        self.editor.tag_configure("number", foreground=self.highlight_num)

        self.write_to_shell("Flow 1.5.0 Engine Initialized...\nReady for instructions.\n" + "-"*40)
        self.update_line_numbers()

    def setup_menus(self):
        self.menu_bar = tk.Menu(self.root)
        file_menu = tk.Menu(self.menu_bar, tearoff=0)
        file_menu.add_command(label="New File", command=lambda: self.editor.delete(1.0, tk.END))
        file_menu.add_command(label="Open...")
        file_menu.add_command(label="Save")
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        self.menu_bar.add_cascade(label="File", menu=file_menu)

        run_menu = tk.Menu(self.menu_bar, tearoff=0)
        run_menu.add_command(label="Run Module (F5)", command=self.run_module)
        self.menu_bar.add_cascade(label="Run", menu=run_menu)
        
        self.root.config(menu=self.menu_bar)

    def on_content_changed(self, event=None):
        self.update_line_numbers()
        self.apply_syntax_highlighting()

    def update_line_numbers(self):
        line_count = self.editor.get('1.0', tk.END).count('\n')
        line_numbers_string = "\n".join(str(i) for i in range(1, line_count + 1))
        self.line_nums.config(state='normal')
        self.line_nums.delete('1.0', tk.END)
        self.line_nums.insert('1.0', line_numbers_string)
        self.line_nums.config(state='disabled')

    def apply_syntax_highlighting(self):
        self.editor.tag_remove("keyword", "1.0", tk.END)
        self.editor.tag_remove("string", "1.0", tk.END)
        self.editor.tag_remove("comment", "1.0", tk.END)
        self.editor.tag_remove("number", "1.0", tk.END)

        content = self.editor.get("1.0", tk.END)
        
        # Highlight Keywords
        keywords = [r'\bCreate\b', r'\bcalled\b', r'\bset\b', r'\bto\b', r'\bShow\b', 
                    r'\bAdd\b', r'\bSubtract\b', r'\bMultiply\b', r'\bDivide\b', r'\bIf\b']
        for kw in keywords:
            for match in re.finditer(kw, content, re.IGNORECASE):
                self.editor.tag_add("keyword", f"1.0 + {match.start()} chars", f"1.0 + {match.end()} chars")

        # Highlight Strings
        for match in re.finditer(r'".*?"', content):
            self.editor.tag_add("string", f"1.0 + {match.start()} chars", f"1.0 + {match.end()} chars")

        # Highlight Comments
        for match in re.finditer(r'Note:.*', content, re.IGNORECASE):
            self.editor.tag_add("comment", f"1.0 + {match.start()} chars", f"1.0 + {match.end()} chars")

        # Highlight Numbers
        for match in re.finditer(r'\b\d+(\.\d+)?\b', content):
            self.editor.tag_add("number", f"1.0 + {match.start()} chars", f"1.0 + {match.end()} chars")

    def outdent(self, event):
        # Implementation for better editor feel
        return "break"

    def write_to_shell(self, text):
        self.shell.config(state='normal')
        self.shell.insert(tk.END, text + "\n")
        self.shell.see(tk.END)
        self.shell.config(state='disabled')

    def run_module(self):
        code = self.editor.get("1.0", tk.END)
        self.write_to_shell(f"\n>>> RESTART: Executing Flow Script...")
        self.interpreter.run(code)

if __name__ == "__main__":
    root = tk.Tk()
    # Attempt to set dark title bar for Mac
    try:
        root.tk.call('tk', 'windowingsystem')
    except: pass
    
    app = FlowPad(root)
    root.mainloop()
