# Deployment Guide - CS2 Skin Changer Web Application

This guide will help you deploy the CS2 Skin Changer web application to various hosting platforms.

## Quick Start (Local Development)

### Windows
1. Double-click `start-web-server.bat`
2. Open browser to `http://localhost:3000`

### Linux/Mac
```bash
./start-web-server.sh
```
Or manually:
```bash
cd web/backend
npm install
npm start
```

## Production Deployment Options

### Option 1: Deploy to Heroku

1. **Install Heroku CLI**:
   ```bash
   curl https://cli-assets.heroku.com/install.sh | sh
   ```

2. **Login to Heroku**:
   ```bash
   heroku login
   ```

3. **Create Heroku app**:
   ```bash
   cd web
   heroku create cs2-skin-changer
   ```

4. **Set environment variables**:
   ```bash
   heroku config:set JWT_SECRET=your-super-secret-key-here
   heroku config:set NODE_ENV=production
   ```

5. **Deploy**:
   ```bash
   git subtree push --prefix web heroku main
   ```

### Option 2: Deploy to Railway

1. **Install Railway CLI**:
   ```bash
   npm install -g @railway/cli
   ```

2. **Login to Railway**:
   ```bash
   railway login
   ```

3. **Initialize project**:
   ```bash
   cd web
   railway init
   ```

4. **Set environment variables**:
   ```bash
   railway variables set JWT_SECRET=your-super-secret-key-here
   railway variables set NODE_ENV=production
   ```

5. **Deploy**:
   ```bash
   railway up
   ```

### Option 3: Deploy to DigitalOcean App Platform

1. **Create a DigitalOcean account** and go to App Platform

2. **Connect your GitHub repository**

3. **Configure the app**:
   - Source Directory: `/web/backend`
   - Build Command: `npm install`
   - Run Command: `npm start`
   - Port: 3000

4. **Set environment variables**:
   - `JWT_SECRET`: (generate a random string)
   - `NODE_ENV`: production
   - `PORT`: 3000

5. **Deploy** and wait for the build

### Option 4: Deploy to VPS (Ubuntu/Debian)

1. **Install Node.js and npm**:
   ```bash
   curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
   sudo apt-get install -y nodejs
   ```

2. **Install PM2** (process manager):
   ```bash
   sudo npm install -g pm2
   ```

3. **Clone repository**:
   ```bash
   git clone https://github.com/pedziito/Skin-Changer.git
   cd Skin-Changer/web/backend
   ```

4. **Install dependencies**:
   ```bash
   npm install --production
   ```

5. **Create .env file**:
   ```bash
   cat > .env << EOF
   PORT=3000
   JWT_SECRET=$(openssl rand -base64 32)
   NODE_ENV=production
   EOF
   ```

6. **Start with PM2**:
   ```bash
   pm2 start server.js --name cs2-skin-changer
   pm2 save
   pm2 startup
   ```

7. **Setup Nginx reverse proxy** (optional):
   ```bash
   sudo apt install nginx
   sudo nano /etc/nginx/sites-available/cs2-skin-changer
   ```
   
   Add configuration:
   ```nginx
   server {
       listen 80;
       server_name your-domain.com;

       location / {
           proxy_pass http://localhost:3000;
           proxy_http_version 1.1;
           proxy_set_header Upgrade $http_upgrade;
           proxy_set_header Connection 'upgrade';
           proxy_set_header Host $host;
           proxy_cache_bypass $http_upgrade;
       }
   }
   ```

   Enable site:
   ```bash
   sudo ln -s /etc/nginx/sites-available/cs2-skin-changer /etc/nginx/sites-enabled/
   sudo nginx -t
   sudo systemctl restart nginx
   ```

### Option 5: Deploy with Docker

1. **Create Dockerfile** in `web/backend/`:
   ```dockerfile
   FROM node:18-alpine

   WORKDIR /app

   COPY package*.json ./
   RUN npm install --production

   COPY . .

   EXPOSE 3000

   CMD ["npm", "start"]
   ```

2. **Create docker-compose.yml** in `web/`:
   ```yaml
   version: '3.8'
   services:
     web:
       build: ./backend
       ports:
         - "3000:3000"
       environment:
         - PORT=3000
         - JWT_SECRET=${JWT_SECRET}
         - NODE_ENV=production
       volumes:
         - ./backend:/app
         - /app/node_modules
       restart: unless-stopped
   ```

3. **Deploy**:
   ```bash
   cd web
   export JWT_SECRET=$(openssl rand -base64 32)
   docker-compose up -d
   ```

## Environment Variables

Required environment variables for production:

| Variable | Description | Example |
|----------|-------------|---------|
| `PORT` | Server port | `3000` |
| `JWT_SECRET` | Secret key for JWT tokens | `supersecretkey123` |
| `NODE_ENV` | Environment mode | `production` |

## Database

The application uses SQLite by default, which creates a `skinchanger.db` file in the backend directory. 

For production, you might want to:
1. **Backup the database regularly**
2. **Use persistent storage** (if deploying to containers)
3. **Consider upgrading to PostgreSQL** for better scalability

## Security Considerations

### Production Checklist

- [ ] Set a strong `JWT_SECRET` (use `openssl rand -base64 32`)
- [ ] Enable HTTPS (use Let's Encrypt or CloudFlare)
- [ ] Set `NODE_ENV=production`
- [ ] Configure CORS to only allow your domain
- [ ] Set up regular database backups
- [ ] Use environment variables for secrets (never commit .env)
- [ ] Enable rate limiting (consider using express-rate-limit)
- [ ] Monitor server logs
- [ ] Keep dependencies updated (`npm audit fix`)

### Recommended: Add Rate Limiting

Add to `server.js`:
```javascript
const rateLimit = require('express-rate-limit');

const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100 // limit each IP to 100 requests per windowMs
});

app.use('/api/', limiter);
```

## Monitoring

### Using PM2 (VPS)
```bash
pm2 logs cs2-skin-changer    # View logs
pm2 status                   # Check status
pm2 restart cs2-skin-changer # Restart app
```

### Using Heroku
```bash
heroku logs --tail
```

### Using Railway
```bash
railway logs
```

## Backup Strategy

### Backup SQLite Database
```bash
# Create backup
cp web/backend/skinchanger.db web/backend/backup-$(date +%Y%m%d).db

# Restore backup
cp web/backend/backup-20240216.db web/backend/skinchanger.db
```

### Automated Backup Script
```bash
#!/bin/bash
DB_PATH="/path/to/web/backend/skinchanger.db"
BACKUP_DIR="/path/to/backups"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR
cp $DB_PATH $BACKUP_DIR/skinchanger_$DATE.db

# Keep only last 7 days of backups
find $BACKUP_DIR -name "skinchanger_*.db" -mtime +7 -delete
```

Add to crontab (daily at 2 AM):
```bash
0 2 * * * /path/to/backup-script.sh
```

## Scaling

### Horizontal Scaling

If you need to scale horizontally:
1. **Replace SQLite with PostgreSQL or MySQL**
2. **Use Redis for session storage**
3. **Deploy multiple instances behind a load balancer**

### Example with PostgreSQL

Install dependencies:
```bash
npm install pg
```

Update connection in `database.js`:
```javascript
const { Pool } = require('pg');

const pool = new Pool({
  connectionString: process.env.DATABASE_URL,
  ssl: { rejectUnauthorized: false }
});
```

## Troubleshooting

### Port already in use
```bash
# Linux/Mac
lsof -ti:3000 | xargs kill -9

# Windows
netstat -ano | findstr :3000
taskkill /PID <PID> /F
```

### Database locked error
- Stop all instances of the server
- Delete the `.db-journal` file if it exists
- Restart the server

### Out of memory errors
- Increase Node.js memory limit: `node --max-old-space-size=4096 server.js`
- Consider upgrading your hosting plan

## Support

For issues or questions:
- Check the [README](../README.md)
- Open an issue on GitHub
- Review the [FAQ](../FAQ.md)

## License

This deployment guide is part of the CS2 Skin Changer project. Educational purposes only.
