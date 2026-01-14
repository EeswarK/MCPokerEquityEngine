# Frontend Deployment Guide

## Vercel Deployment

### Prerequisites

- Vercel account
- GitHub repository (or other Git provider)

### Steps

1. **Connect Repository**

   - Go to Vercel dashboard
   - Click "Add New Project"
   - Import your Git repository
   - Select root directory: `src/web`

2. **Configure Build Settings**

   - Framework Preset: Vite
   - Build Command: `npm run build`
   - Output Directory: `dist`
   - Install Command: `npm install`

3. **Set Environment Variables**
   In Vercel project settings, add:

   - `VITE_API_URL`: Your EC2 backend URL (e.g., `http://your-ec2-ip:8000`)
   - `VITE_WS_URL`: Your WebSocket URL (e.g., `ws://your-ec2-ip:8000`)
   - `VITE_USE_MOCK`: `false` (for production)

4. **Deploy**
   - Click "Deploy"
   - Vercel will build and deploy automatically

### Custom Domain (Optional)

1. Go to Project Settings > Domains
2. Add your custom domain
3. Configure DNS as instructed

### Environment-Specific Configuration

- **Production**: Use real backend URLs
- **Preview**: Can use mock backend or staging backend
- **Development**: Uses mock backend by default

## Local Development

```bash
cd src/web
npm install
npm run dev
```

Access at http://localhost:3000

## Environment Variables

### Frontend

Create a `.env` file in `src/web/` (or use `.env.example` as a template):

```bash
# API Configuration
VITE_API_URL=http://localhost:8000
VITE_WS_URL=ws://localhost:8000

# Development
VITE_USE_MOCK=true
```

### Backend (Python API)

- `TELEMETRY_PORT` (default: 8001): Port for C++ telemetry collector WebSocket server (only used for direct connections, not when proxied through nginx)
- `TELEMETRY_HOST` (default: localhost): Hostname for telemetry WebSocket URL in job responses (use nginx hostname when proxied)
- `TELEMETRY_WS_PROTOCOL` (default: ws): WebSocket protocol - use "wss" when proxied through nginx with SSL
- `TELEMETRY_COLLECTOR_BINARY`: Path to telemetry collector executable (default: relative path in source)

### Python API

- `API_PORT` (default: 8000): Port for Python FastAPI server

## Troubleshooting

### Build Fails

- Check Node.js version (should be 18+)
- Verify all dependencies install correctly
- Check build logs in Vercel dashboard

### API Connection Issues

- Verify `VITE_API_URL` is set correctly
- Check CORS configuration on backend
- Verify backend is accessible from Vercel deployment

### WebSocket Connection Issues

- Verify `VITE_WS_URL` uses `ws://` or `wss://` correctly
- Check firewall/security group settings
- Verify backend WebSocket endpoint is accessible
