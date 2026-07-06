import re
from pathlib import Path

Import("env") # type: ignore

SECRETS_PATH = Path(env["PROJECT_DIR"]) / "include" / "config" / "secrets.h" # type: ignore


def _load_ota_password(path: Path) -> str:
    if not path.exists():
        raise RuntimeError(f"Missing secrets file: {path}")

    text = path.read_text(encoding="utf-8")
    match = re.search(r'SECRET_OTA_PASSWORD\[\]\s*=\s*"([^"]+)"\s*;', text)
    if match is None:
        raise RuntimeError("SECRET_OTA_PASSWORD not found in include/config/secrets.h")

    password = match.group(1).strip()
    if not password:
        raise RuntimeError("SECRET_OTA_PASSWORD is empty in include/config/secrets.h")

    return password


ota_password = _load_ota_password(SECRETS_PATH)
env.Append(UPLOAD_FLAGS=[f"--auth={ota_password}"]) # type: ignore
