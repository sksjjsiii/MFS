import os, subprocess, shutil, json, sys
from pathlib import Path
from fastmcp import FastMCP

BASE = os.environ.get("MCP_ALLOWED_DIR", os.getcwd())
TIMEOUT = int(os.environ.get("MCP_COMMAND_TIMEOUT", "120"))

mcp = FastMCP("System Control MCP")

def _p(path):
    p = Path(path).expanduser().resolve()
    return p

@mcp.tool()
def read_file(path: str, start_line: int = 0, end_line: int = 0) -> str:
    """خواندن محتوای فایل."""
    p = _p(path)
    text = p.read_text(encoding="utf-8", errors="replace")
    if start_line or end_line:
        lines = text.splitlines()
        end = end_line if end_line else len(lines)
        return "\n".join(lines[start_line:end])
    return text

@mcp.tool()
def write_file(path: str, content: str) -> str:
    """نوشتن یا بازنویسی فایل."""
    p = _p(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(content, encoding="utf-8")
    return f"OK: {p} ({len(content)} chars)"

@mcp.tool()
def append_file(path: str, content: str) -> str:
    """افزودن به انتهای فایل."""
    p = _p(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open("a", encoding="utf-8") as f:
        f.write(content)
    return f"OK: appended to {p}"

@mcp.tool()
def edit_file(path: str, old_text: str, new_text: str) -> str:
    """جایگزینی متن در فایل."""
    p = _p(path)
    text = p.read_text(encoding="utf-8")
    if old_text not in text:
        return "ERROR: text not found"
    p.write_text(text.replace(old_text, new_text, 1), encoding="utf-8")
    return f"OK: edited {p}"

@mcp.tool()
def delete_file(path: str) -> str:
    """حذف فایل."""
    p = _p(path)
    p.unlink()
    return f"OK: deleted {p}"

@mcp.tool()
def list_directory(path: str = ".") -> str:
    """لیست محتویات دایرکتوری."""
    p = _p(path)
    out = []
    for item in sorted(p.iterdir()):
        tag = "DIR " if item.is_dir() else "FILE"
        size = item.stat().st_size if item.is_file() else 0
        out.append(f"{tag} {item.name} ({size})")
    return "\n".join(out) if out else "(empty)"

@mcp.tool()
def create_directory(path: str) -> str:
    """ایجاد دایرکتوری."""
    _p(path).mkdir(parents=True, exist_ok=True)
    return f"OK: created {path}"

@mcp.tool()
def search_files(pattern: str, path: str = ".") -> str:
    """جستجوی فایل بر اساس الگو."""
    p = _p(path)
    m = [str(x) for x in p.rglob(pattern)]
    return "\n".join(m[:200]) if m else "no match"

@mcp.tool()
def run_command(command: str, workdir: str = "") -> str:
    """اجرای دستور shell."""
    cwd = workdir if workdir else BASE
    r = subprocess.run(command, shell=True, cwd=cwd,
                       capture_output=True, text=True, timeout=TIMEOUT)
    out = r.stdout or ""
    if r.stderr:
        out += f"\n--- stderr ---\n{r.stderr}"
    out += f"\n(exit={r.returncode})"
    return out

@mcp.tool()
def run_python(code: str) -> str:
    """اجرای کد پایتون."""
    r = subprocess.run([sys.executable, "-c", code],
                       capture_output=True, text=True, timeout=TIMEOUT, cwd=BASE)
    out = r.stdout or ""
    if r.stderr:
        out += f"\n--- stderr ---\n{r.stderr}"
    return out

@mcp.tool()
def system_info() -> str:
    """اطلاعات سیستم."""
    return json.dumps({
        "cwd": os.getcwd(),
        "disk": shutil.disk_usage("/")._asdict(),
        "env": dict(list(os.environ.items())[:30]),
    }, indent=2, ensure_ascii=False)

@mcp.tool()
def get_env(key: str) -> str:
    """خواندن متغیر محیطی."""
    return f"{key}={os.environ.get(key, '<unset>')}"

@mcp.tool()
def git_status() -> str:
    """وضعیت Git."""
    return run_command("git status --short --branch")

@mcp.tool()
def git_diff() -> str:
    """دیف Git."""
    return run_command("git diff HEAD")

@mcp.tool()
def git_add_commit(message: str, files: str = ".") -> str:
    """Git add + commit."""
    a = run_command(f"git add {files}")
    c = run_command(f'git commit -m "{message}"')
    return a + "\n" + c

@mcp.tool()
def http_request(url: str, method: str = "GET", data: str = "", headers: str = "") -> str:
    """درخواست HTTP با curl."""
    cmd = f"curl -s -X {method} "
    if data:
        cmd += f"-d '{data}' "
    if headers:
        cmd += f"-H '{headers}' "
    cmd += f"'{url}'"
    return run_command(cmd)

if __name__ == "__main__":
    mcp.run(transport="streamable-http", host="0.0.0.0", port=5000)
