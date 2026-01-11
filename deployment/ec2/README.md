# EC2 Deployment Guide

## Prerequisites

- AWS EC2 instance (Ubuntu 22.04 LTS recommended)
- Instance type: t3.medium (2 vCPU, 4 GB RAM) minimum
- Security group configured (see Security Group Configuration below)
- SSH access to instance

## Security Group Configuration

Configure your EC2 security group to allow:

- **SSH (port 22)**: From your IP address only
- **HTTP (port 8000)**: From your frontend deployment origin (Vercel/Netlify IP ranges) or 0.0.0.0/0 for testing
- **WebSocket (port 8000)**: Same as HTTP

### Example Security Group Rules

| Type       | Protocol | Port | Source                             |
| ---------- | -------- | ---- | ---------------------------------- |
| SSH        | TCP      | 22   | Your IP /32                        |
| Custom TCP | TCP      | 8000 | 0.0.0.0/0 (restrict in production) |

## Deployment Steps

### 1. Launch EC2 Instance

- AMI: Ubuntu Server 22.04 LTS (ami-0c55b159cbfafe1f0 in us-east-1)
- Instance Type: t3.medium or larger
- Storage: 20 GB GP3 minimum
- Security Group: Configure as above

### 2. Connect to Instance

```bash
ssh -i your-key.pem ubuntu@your-instance-ip
```

### 3. Prepare Application Files

Option A: Clone from Git repository

```bash
cd /tmp
git clone <repository-url> poker-engine
```

Option B: Copy files via SCP

```bash
scp -i your-key.pem -r /path/to/PokerEquityEngine ubuntu@your-instance-ip:/tmp/poker-engine
```

### 4. Run Setup Script

```bash
cd /tmp/poker-engine
sudo chmod +x deployment/ec2/setup.sh
sudo ./deployment/ec2/setup.sh
```

The script will:

- Install Python 3.10+
- Create application user (`poker-engine`)
- Setup virtual environment
- Install dependencies
- Configure systemd service
- Setup firewall
- Start the service

### 5. Verify Deployment

```bash
# Check service status
sudo systemctl status poker-engine

# Check logs
sudo journalctl -u poker-engine -f

# Test health endpoint
curl http://localhost:8000/health

# Test from external IP
curl http://your-ec2-ip:8000/health
```

## Version Management

### Application Versioning

The application version is defined in:

- `pyproject.toml`: `version = "0.1.0"`
- API response: `/health` endpoint returns version

### Dependency Versioning

Production dependencies are pinned in:

- `deployment/ec2/requirements.txt`: Exact versions for reproducibility

Development dependencies use `>=` in:

- `requirements.txt`: Flexible for development
- `pyproject.toml`: Flexible for development

### Updating Dependencies

To update dependencies:

1. Update `deployment/ec2/requirements.txt` with new versions
2. Test locally: `pip install -r deployment/ec2/requirements.txt`
3. Deploy to EC2 and run setup script again
4. Restart service: `sudo systemctl restart poker-engine`

## Service Management

### Start Service

```bash
sudo systemctl start poker-engine
```

### Stop Service

```bash
sudo systemctl stop poker-engine
```

### Restart Service

```bash
sudo systemctl restart poker-engine
```

### View Logs

```bash
# Follow logs
sudo journalctl -u poker-engine -f

# Last 50 lines
sudo journalctl -u poker-engine -n 50

# Since boot
sudo journalctl -u poker-engine -b
```

### Check Status

```bash
sudo systemctl status poker-engine
```

## Updating Application

```bash
cd /opt/poker-engine
sudo -u poker-engine git pull
sudo -u poker-engine bash -c "source venv/bin/activate && pip install -r deployment/ec2/requirements.txt && pip install -e ."
sudo systemctl restart poker-engine
```

## Troubleshooting

### Service Won't Start

1. Check logs: `sudo journalctl -u poker-engine -n 50`
2. Verify Python path: `which python3.10`
3. Check permissions: `ls -la /opt/poker-engine`
4. Verify virtual environment: `ls -la /opt/poker-engine/venv/bin/uvicorn`

### Port Already in Use

```bash
# Check what's using port 8000
sudo lsof -i :8000

# Kill process if needed
sudo kill <PID>
```

### Connection Refused from Frontend

1. Verify security group allows port 8000
2. Check firewall: `sudo ufw status`
3. Verify service is running: `sudo systemctl status poker-engine`
4. Test locally: `curl http://localhost:8000/health`
5. Test from external: `curl http://your-ec2-ip:8000/health`

### WebSocket Connection Fails

1. Verify WebSocket endpoint: `websocat ws://your-ec2-ip:8000/ws/test-job-id`
2. Check security group allows WebSocket connections
3. Verify frontend uses correct WebSocket URL (ws:// or wss://)
4. Check browser console for connection errors

## Security Considerations

### Production Hardening

1. **Restrict Security Group**: Only allow port 8000 from frontend origin IPs
2. **Use HTTPS/WSS**: Deploy behind Application Load Balancer with SSL certificate
3. **Rate Limiting**: Implement rate limiting (future phase)
4. **Authentication**: Add authentication/authorization (future phase)
5. **Logging**: Monitor logs for suspicious activity
6. **Updates**: Keep system packages updated: `sudo apt-get update && sudo apt-get upgrade`

## Monitoring

### Health Checks

The `/health` endpoint can be used for health checks:

```bash
curl http://your-ec2-ip:8000/health
```

Expected response:

```json
{ "status": "healthy", "version": "0.1.0" }
```

### Resource Monitoring

Monitor EC2 instance metrics:

- CPU utilization
- Memory usage
- Network I/O
- Disk I/O

Use CloudWatch or similar monitoring tools.

## Backup and Recovery

### Application Files

Application files are stored in `/opt/poker-engine`. To backup:

```bash
sudo tar -czf poker-engine-backup.tar.gz /opt/poker-engine
```

### Recovery

To restore from backup:

```bash
sudo tar -xzf poker-engine-backup.tar.gz -C /
sudo systemctl restart poker-engine
```
