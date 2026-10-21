import lab_check
from labrepo import ROOT


def write(tmp_path, monkeypatch, rel, text):
    monkeypatch.setattr(lab_check, "ROOT", tmp_path)
    path = tmp_path / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    return [path]


def test_flags_literal_pins_and_stray_serial(tmp_path, monkeypatch):
    files = write(
        tmp_path,
        monkeypatch,
        "projects/p001-x/src/main.cpp",
        "pinMode(4, OUTPUT);\nFastLED.addLeds<WS2812B, 5, GRB>(l, 8);\nSerial.println(1);\n"
        "digitalWrite(BOARD_STATUS_LED, LOW);  // pin 10\npinMode(7, OUTPUT);  // lab:allow\n",
    )
    errors = lab_check.check_code(files)
    assert [e.split(":")[1] for e in errors] == ["1", "2", "3"]


def test_config_header_may_not_hardcode_pins(tmp_path, monkeypatch):
    files = write(
        tmp_path, monkeypatch, "projects/p001-x/include/config.h", "#define LED_DATA_PIN 5\n"
    )
    assert len(lab_check.check_code(files)) == 1


def test_secrets_flagged_unless_placeholder(tmp_path, monkeypatch):
    key = "WIFI_" + "PASSWORD"
    files = write(
        tmp_path,
        monkeypatch,
        "projects/p001-x/include/net.h",
        f'#define {key} "hunter2"\n#define {key} "changeme"\nconst char* ssid = "";\n',
    )
    errors = lab_check.check_secrets(files)
    assert len(errors) == 1 and ":1:" in errors[0]


def test_secrets_header_must_not_be_committed(tmp_path, monkeypatch):
    files = write(tmp_path, monkeypatch, "projects/p001-x/include/secrets.h", "")
    assert "untracked" in lab_check.check_secrets(files)[0]


def test_repo_passes_its_own_rules():
    assert ROOT.is_dir()
    assert lab_check.check_boards() == []
    assert lab_check.check_projects() == []


def test_more_pin_literal_forms(tmp_path, monkeypatch):
    lines = [
        "Wire.begin(21, 22);",
        "SPI.begin(18, 19, 23, 5);",
        "Serial1.begin(115200, SERIAL_8N1, 16, 17);",
        "cfg.bck_io_num = 26;",
        "#define LED_PIN (5)",
        "constexpr int kLedPin = 5;",
        "Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);",  # ok
        "Serial1.begin(115200);",  # ok: no pins
    ]
    files = write(tmp_path, monkeypatch, "projects/p001-x/src/main.cpp", "\n".join(lines) + "\n")
    flagged = sorted(int(e.split(":")[1]) for e in lab_check.check_code(files))
    assert flagged == [1, 2, 3, 4, 5, 6]


def test_serial_in_headers_and_other_serials(tmp_path, monkeypatch):
    files = write(
        tmp_path, monkeypatch, "projects/p001-x/include/log.h",
        'USBSerial.println("x");\nets_printf("x");\n',
    )  # fmt: skip
    assert len(lab_check.check_code(files)) == 2


def test_secret_forms(tmp_path, monkeypatch):
    k = "WIFI_" + "PASS"
    files = write(
        tmp_path, monkeypatch, "projects/p001-x/include/net.h",
        f'#define {k} "hunter2"\nconst char* pwd = \'hunter2\';\n'
        'const char* kTokenUrl = "https://example.com";\n#define SSID_PREFIX "lab-"\n',
    )  # fmt: skip
    errors = lab_check.check_secrets(files)
    assert sorted(int(e.split(":")[1]) for e in errors) == [1, 2]
    yaml_files = write(
        tmp_path, monkeypatch, "inventory/x.yaml",
        "wifi_" + "password: hunter2\npassword: ${{ secrets.GITHUB_TOKEN }}\n",
    )  # fmt: skip
    assert len(lab_check.check_secrets(yaml_files)) == 1
