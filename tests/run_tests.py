"""Build and test without touching real application data. Python 3, GCC required."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
BUILD.mkdir(exist_ok=True)
CC = os.environ.get("CC", "gcc")
EXT = ".exe" if os.name == "nt" else ""
APP = BUILD / ("carbon-credit" + EXT)
ORDERS = BUILD / ("test-orders" + EXT)
FLAGS = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2"]
if os.environ.get("SANITIZE") == "1":
    FLAGS += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]


def compile_source(source, destination):
    subprocess.run([CC, *FLAGS, str(ROOT / source), "-o", str(destination)], check=True)


def run(cwd, lines=(), expected=0):
    result = subprocess.run([str(APP)], cwd=cwd, input="\n".join(lines) + "\n",
                            text=True, capture_output=True, timeout=10)
    assert result.returncode == expected, result.stdout + result.stderr
    return result.stdout + result.stderr


def demo(cwd):
    for name in ("users.txt", "projects.txt", "transaction.txt"):
        shutil.copyfile(ROOT / "demo" / (name + ".example"), Path(cwd) / name)


compile_source("login.c", APP)
compile_source("tests/test_orders.c", ORDERS)
with tempfile.TemporaryDirectory(prefix="carbon-orders-") as folder:
    subprocess.run([str(ORDERS)], cwd=folder, check=True, timeout=20)

with tempfile.TemporaryDirectory(prefix="carbon-cli-") as folder:
    folder = Path(folder)
    # First-run registration, creation, editing, cancellation of edits and deletion.
    output = run(folder, ["nonsense", "2", "sample_seller", "demo123", "demo123",
                          "school", "hospital", "pet", "sample@example.test", "100", "0", "0", "",
                          "1", "sample_seller", "demo123", "2", "1", "Sample Forest", "",
                          "4", "30", "200", "2", "", "example.test/reference", "1", "",
                          "2", "1", "3", "5", "Custom Forestry", "10",
                          "2", "1", "4", "50", "10",
                          "2", "1", "4", "10", "11",
                          "4", "5", "3"])
    assert "Registration successful" in output and "Project 0001 saved" in output, output
    assert '"Custom Forestry",50.00,200.00' in (folder / "projects.txt").read_text()
    assert "50.00 0.00" in (folder / "users.txt").read_text()
    output = run(folder, ["1", "sample_seller", "demo123", "1", "5", "0", "5", "3"])
    assert "Sample Forest" in output and "Custom Forestry" in output, output
    # Password recovery must preserve all eight account fields and allow login after restart.
    output = run(folder, ["1", "sample_seller", "forget", "2", "hospital", "changed123",
                          "changed123", "sample_seller", "changed123", "5", "3"])
    assert "Password updated" in output and "Login successful" in output, output
    fields = (folder / "users.txt").read_text().split()
    assert fields == ["sample_seller", "changed123", "school", "hospital", "pet", "50.00", "0.00", "sample@example.test"]
    output = run(folder, ["1", "sample_seller", "changed123", "2", "3", "1", "1", "4", "5", "3"])
    assert "Project deleted" in output, output
    assert "100.00 0.00" in (folder / "users.txt").read_text()
    assert not (folder / "projects.txt").read_text()
    print("PASS: CLI registration, creation, type/credit editing, cancel, reload, recovery and deletion")

with tempfile.TemporaryDirectory(prefix="carbon-demo-") as folder:
    demo(folder)
    output = run(folder, ["1", "buyer", "demo123", "1", "5", "1", "1", "", "5", "3"])
    assert "Purchase requested" in output, output
    output = run(folder, ["1", "seller", "demo123", "3", "1", "1", "5", "3"])
    assert "Order updated" in output, output
    output = run(folder, ["1", "buyer", "demo123", "3", "1", "3", "5", "3"])
    assert "Order updated" in output, output
    assert not (Path(folder) / "transaction.txt").read_text()
    print("PASS: CLI purchase, seller confirmation and buyer acknowledgement across separate processes")

for name, content in [
    ("users.txt", "x" * 5000),
    ("users.txt", "a b c d e nan 100 a@example.test\n"),
    ("projects.txt", '"1","broken\n'),
    ("transaction.txt", "1,forest,missing,buyer,Pending\n"),
    ("transaction.txt", "1,forest,seller,buyer,Pending\n" * 101),
]:
    with tempfile.TemporaryDirectory(prefix="carbon-invalid-") as folder:
        demo(folder)
        target = Path(folder) / name
        target.write_text(content)
        before = {p.name: p.read_bytes() for p in Path(folder).iterdir()}
        output = run(folder, ["3"], expected=1)
        assert "Error:" in output
        assert before == {p.name: p.read_bytes() for p in Path(folder).iterdir()}
print("PASS: invalid records stop before saving or overwriting data")

with tempfile.TemporaryDirectory(prefix="carbon-eof-") as folder:
    result = subprocess.run([str(APP)], cwd=folder, input="", text=True,
                            capture_output=True, timeout=5, check=True)
    run(folder, ["invalid", "-1", "999999999999999999", "3"])
print("PASS: EOF and invalid menu input exit or recover without hanging")
print("All checks passed.")
