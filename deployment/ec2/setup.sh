#!/bin/bash
set -euo pipefail

echo "=== Poker Equity Engine EC2 Setup ==="

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo)"
   exit 1
fi

# Detect OS
if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    OS=$ID
    VERSION=$VERSION_ID
else
    echo "Cannot detect OS"
    exit 1
fi

echo "Detected OS: $OS $VERSION"

# Update system packages
echo "Updating system packages..."
apt-get update
apt-get upgrade -y

# Install Python 3.10+
echo "Installing Python 3.10+..."
apt-get install -y software-properties-common
add-apt-repository -y ppa:deadsnakes/ppa || true
apt-get update
apt-get install -y python3.10 python3.10-venv python3.10-dev python3-pip

# Install system dependencies
echo "Installing system dependencies..."
apt-get install -y build-essential git curl ufw

# Create application user
echo "Creating application user..."
if ! id "poker-engine" &>/dev/null; then
    useradd -m -s /bin/bash poker-engine
    echo "User 'poker-engine' created"
else
    echo "User 'poker-engine' already exists"
fi

# Create application directory
APP_DIR="/opt/poker-engine"
echo "Setting up application directory: $APP_DIR"
mkdir -p "$APP_DIR"
chown poker-engine:poker-engine "$APP_DIR"

# Clone or copy application files
# Assumes repository is cloned to /tmp/poker-engine or files are available
if [[ -d "/tmp/poker-engine" ]]; then
    echo "Copying application files..."
    shopt -s dotglob
    cp -r /tmp/poker-engine/* "$APP_DIR/" 2>/dev/null || {
        echo "Copy failed, trying alternative method..."
        find /tmp/poker-engine -mindepth 1 -maxdepth 1 -exec cp -r {} "$APP_DIR/" \;
    }
    shopt -u dotglob
    chown -R poker-engine:poker-engine "$APP_DIR"
elif [[ -f "$APP_DIR/pyproject.toml" ]]; then
    echo "Application files already present in $APP_DIR"
else
    echo "ERROR: Application files not found in /tmp/poker-engine"
    echo "Please ensure the repository is cloned or files are available before proceeding"
    exit 1
fi

# Setup virtual environment
echo "Setting up Python virtual environment..."
REQUIREMENTS_FILE="$APP_DIR/deployment/ec2/requirements.txt"
if [[ ! -f "$REQUIREMENTS_FILE" ]]; then
    echo "ERROR: Requirements file not found at $REQUIREMENTS_FILE"
    echo "Please ensure the repository was copied correctly to $APP_DIR"
    exit 1
fi

if [[ ! -f "$APP_DIR/pyproject.toml" ]]; then
    echo "ERROR: pyproject.toml not found at $APP_DIR/pyproject.toml"
    echo "Please ensure the repository was copied correctly to $APP_DIR"
    exit 1
fi

sudo -u poker-engine bash <<EOF
cd "$APP_DIR"
python3.10 -m venv venv
source venv/bin/activate
pip install --upgrade pip setuptools wheel
pip install -r "$REQUIREMENTS_FILE"
pip install -e .
EOF

# Install systemd service
echo "Installing systemd service..."
cat > /etc/systemd/system/poker-engine.service <<'SERVICE_EOF'
[Unit]
Description=Poker Equity Engine API Server
After=network.target

[Service]
Type=simple
User=poker-engine
Group=poker-engine
WorkingDirectory=/opt/poker-engine
Environment="PATH=/opt/poker-engine/venv/bin"
Environment="PYTHONUNBUFFERED=1"
ExecStart=/opt/poker-engine/venv/bin/uvicorn src.python.api.server:app --host 0.0.0.0 --port 8000
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
SERVICE_EOF

# Reload systemd and enable service
systemctl daemon-reload
systemctl enable poker-engine.service

# Configure firewall
echo "Configuring firewall..."
ufw --force enable
ufw allow 22/tcp
ufw allow 8000/tcp
ufw allow 8001/tcp
ufw status

# Start service
echo "Starting poker-engine service..."
systemctl start poker-engine.service

# Wait a moment for service to start
sleep 2

# Check service status
if systemctl is-active --quiet poker-engine.service; then
    echo "✓ Service started successfully"
else
    echo "✗ Service failed to start. Check logs with: sudo journalctl -u poker-engine -n 50"
    exit 1
fi

echo ""
echo "=== Setup Complete ==="
echo "Service status: sudo systemctl status poker-engine"
echo "View logs: sudo journalctl -u poker-engine -f"
echo "Test API: curl http://localhost:8000/health"
