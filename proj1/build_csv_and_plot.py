import re
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

CONFIG_MAP = {
    (128,  64,   32):  "config1",
    (256,  128,  64):  "config2",
    (512,  256,  128): "config3",
    (1024, 512,  256): "config4",
    (2048, 1024, 512): "config5",
    (4096, 2048, 1024):"config6",
}

def grab_int(txt, pattern):
    m = re.search(pattern, txt, flags=re.MULTILINE)
    return int(m.group(1)) if m else None

def grab_float(txt, pattern):
    m = re.search(pattern, txt, flags=re.MULTILINE)
    return float(m.group(1)) if m else None

def grab_str(txt, pattern):
    m = re.search(pattern, txt, flags=re.MULTILINE)
    return m.group(1).strip() if m else None

def infer_mode(pbp, pdc, ptc, mdp_name):
    # Perfect flags: 1=perfect, 0=real
    # Modes per Project 1:
    # perfALL: pbp=1,pdc=1,ptc=1 and mdp=oracle
    # realD$:  pdc=0 (others perfect) and mdp=oracle
    # realBP:  pbp=0 (others perfect) and mdp=oracle
    # noT$:    ptc=0 (others perfect) and mdp=oracle
    # realMDP-* : pbp=pdc=ptc=1 and mdp != oracle

    mdp = (mdp_name or "").lower()

    if pbp == 0 and pdc == 1 and ptc == 1 and "oracle" in mdp:
        return "realBP"
    if pdc == 0 and pbp == 1 and ptc == 1 and "oracle" in mdp:
        return "realD$"
    if ptc == 0 and pbp == 1 and pdc == 1 and "oracle" in mdp:
        return "noT$"
    if pbp == 1 and pdc == 1 and ptc == 1 and "oracle" in mdp:
        return "perfALL"

    # Memory dependence predictor cases (names as printed in logs)
    if "store sets" in mdp:
        return "realMDP-SS"
    if "always pred. no conflict" in mdp:
        return "realMDP-APNC"
    if "always pred. conflict" in mdp and "synchronize" in mdp:
        # Covers APC variants; your project uses synchronize through IQ
        return "realMDP-APC"

    return "unknown"

def parse_one_log(path: Path):
    txt = path.read_text(errors="ignore")

    # IPC (robust to "ipc_rate : 2.46")
    ipc = grab_float(txt, r"^\s*ipc_rate\s*[: ]\s*([0-9]+(?:\.[0-9]+)?)\s*$")

    # Structure sizes
    al = grab_int(txt, r"^\s*ACTIVE LIST\s*=\s*(\d+)\s*$")
    iq = grab_int(txt, r"^\s*ISSUE QUEUE\s*=\s*(\d+)\s*$")
    lq = grab_int(txt, r"^\s*LOAD QUEUE\s*=\s*(\d+)\s*$")

    # Perfect flags
    pbp = grab_int(txt, r"^PERFECT_BRANCH_PRED\s*=\s*(\d+)\s*$")
    pdc = grab_int(txt, r"^PERFECT_DCACHE\s*=\s*(\d+)\s*$")
    ptc = grab_int(txt, r"^PERFECT_TRACE_CACHE\s*=\s*(\d+)\s*$")

    # MDP name line (used to distinguish realMDP modes)
    mdp_name = grab_str(txt, r"^\s*MEMORY DEPENDENCE PREDICTOR:\s*(.+?)\s*$")

    mode = infer_mode(pbp, pdc, ptc, mdp_name)

    config = CONFIG_MAP.get((al, lq, iq), "unknown")

    return {
        "stats_file": path.name,
        "ipc_rate": ipc,
        "mode": mode,
        "config": config,
        "al": al,
        "lq": lq,
        "iq": iq,
        "mdp": mdp_name,
        "pbp": pbp,
        "pdc": pdc,
        "ptc": ptc,
    }

def build_csv_and_plot(folder: str, out_csv: str, title: str):
    folder_path = Path(folder)
    rows = [parse_one_log(p) for p in sorted(folder_path.glob("stats.*"))]
    df = pd.DataFrame(rows)

    # Sanity checks
    df.to_csv(out_csv, index=False)
    print(f"Wrote {out_csv} with {len(df)} rows")
    print(df["mode"].value_counts(dropna=False))
    print(df["config"].value_counts(dropna=False))

    # Plot (only known mode/config)
    df2 = df[(df["mode"] != "unknown") & (df["config"] != "unknown")].copy()
    order = ["config1","config2","config3","config4","config5","config6"]

    plt.figure(figsize=(9,6))
    for mode in ["perfALL","realD$","realBP","noT$","realMDP-APC","realMDP-APNC","realMDP-SS"]:
        sub = df2[df2["mode"] == mode].set_index("config")
        if all(c in sub.index for c in order):
            y = [sub.loc[c, "ipc_rate"] for c in order]
            plt.plot(order, y, marker="o", label=mode)

    plt.xlabel("Config (AL/LQ/IQ sizes)")
    plt.ylabel("IPC")
    plt.title(title)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"{title}.png", dpi=200)
    plt.close()

if __name__ == "__main__":
    build_csv_and_plot("p1_logs/bzip2", "bzip2.csv", "401.bzip2_chicken_ref")
    build_csv_and_plot("p1_logs/lbm", "lbm.csv", "619.lbm_s_ref")
    build_csv_and_plot("p1_logs/perlbench", "perlbench.csv", "400.perlbench_splitmail_ref")
