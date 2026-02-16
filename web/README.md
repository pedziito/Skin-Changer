# CS2 Skin Changer - Web Application

This is the web-based version of CS2 Skin Changer that allows users to manage their skin configurations through a beautiful web interface.

## Features

- 🔐 User authentication (Login/Signup)
- 🎨 Beautiful, modern UI with gradient design
- 📱 Fully responsive (works on mobile and desktop)
- 🔑 API token generation for desktop client
- ⚡ Real-time skin configuration management
- 💾 Persistent storage with SQLite database
- 🚀 Easy deployment

## Architecture

### Backend
- **Framework**: Express.js (Node.js)
- **Database**: SQLite3
- **Authentication**: JWT (JSON Web Tokens)
- **Password Hashing**: bcryptjs

### Frontend
- **Styling**: Modern CSS with gradients and animations
- **JavaScript**: Vanilla JS (no framework dependencies)
- **Responsive**: Mobile-first design
- **Fonts**: Inter font family

## Installation

### Prerequisites
- Node.js 14+ installed
- npm or yarn package manager

### Setup

1. **Navigate to backend directory**:
   ```bash
   cd web/backend
   ```

2. **Install dependencies**:
   ```bash
   npm install
   ```

3. **Create environment file**:
   ```bash
   cp .env.example .env
   ```

4. **Edit .env file** (optional):
   ```
   PORT=3000
   JWT_SECRET=your-secret-key-here
   NODE_ENV=production
   ```

5. **Start the server**:
   ```bash
   npm start
   ```

   Or for development with auto-reload:
   ```bash
   npm run dev
   ```

6. **Access the application**:
   Open your browser and go to: `http://localhost:3000`

## Usage

### 1. Create an Account
- Click "Sign Up" on the login page
- Enter your username, email, and password
- Click "Create Account"

### 2. Login
- Enter your username and password
- Click "Sign In"

### 3. Generate API Token
- Once logged in, click "Generate Token" in the API Token section
- Copy the generated token
- Use this token in the CS2 Skin Changer desktop client

### 4. Configure Skins
- Select a weapon category (Rifles, Pistols, etc.)
- Choose a specific weapon
- Pick your desired skin
- Click "Apply Skin" to save the configuration

### 5. View Configurations
- All your saved skin configurations appear at the bottom
- You can delete individual configurations
- Use "Reset All" to clear all configurations

## API Endpoints

### Authentication
- `POST /api/auth/register` - Register new user
- `POST /api/auth/login` - Login user
- `GET /api/auth/verify` - Verify JWT token
- `POST /api/auth/generate-api-token` - Generate API token for desktop client

### Configuration
- `GET /api/config` - Get all user configurations
- `POST /api/config` - Create or update configuration
- `GET /api/config/:id` - Get specific configuration
- `DELETE /api/config/:id` - Delete specific configuration
- `DELETE /api/config` - Delete all configurations

## Database Schema

### Users Table
```sql
CREATE TABLE users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  username TEXT UNIQUE NOT NULL,
  email TEXT UNIQUE NOT NULL,
  password TEXT NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  last_login DATETIME
);
```

### Skin Configurations Table
```sql
CREATE TABLE skin_configs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id INTEGER NOT NULL,
  weapon_category TEXT NOT NULL,
  weapon_name TEXT NOT NULL,
  weapon_id INTEGER NOT NULL,
  skin_name TEXT NOT NULL,
  paint_kit INTEGER NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(id)
);
```

### API Tokens Table
```sql
CREATE TABLE api_tokens (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  user_id INTEGER NOT NULL,
  token TEXT UNIQUE NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  expires_at DATETIME,
  last_used DATETIME,
  FOREIGN KEY (user_id) REFERENCES users(id)
);
```

## Security Features

- ✅ Password hashing with bcrypt
- ✅ JWT authentication
- ✅ CORS enabled
- ✅ SQL injection prevention
- ✅ XSS protection
- ✅ Environment variable configuration

## Deployment

### Local Development
```bash
cd web/backend
npm run dev
```

### Production
```bash
cd web/backend
npm start
```

### Environment Variables
Create a `.env` file with:
```
PORT=3000
JWT_SECRET=change-this-to-a-secure-random-string
NODE_ENV=production
```

## File Structure

```
web/
├── backend/
│   ├── routes/
│   │   ├── auth.js          # Authentication routes
│   │   └── config.js        # Configuration routes
│   ├── database.js          # Database initialization
│   ├── server.js            # Main server file
│   ├── package.json         # Dependencies
│   └── .env.example         # Environment template
└── frontend/
    ├── css/
    │   └── style.css        # Application styling
    ├── js/
    │   ├── api.js           # API helper functions
    │   ├── auth.js          # Authentication logic
    │   └── dashboard.js     # Dashboard functionality
    ├── config/
    │   └── skins.json       # Skin database
    └── index.html           # Main HTML file
```

## Troubleshooting

### Port already in use
If port 3000 is already in use, change the PORT in `.env` file:
```
PORT=8080
```

### Database errors
Delete the database file and restart:
```bash
rm web/backend/skinchanger.db
npm start
```

### Authentication issues
Clear browser localStorage:
```javascript
// In browser console:
localStorage.clear();
```

## Important Warnings

⚠️ **This tool violates Valve's Terms of Service**
⚠️ **Using this will likely result in a VAC ban**
⚠️ **Educational purposes only**
⚠️ **Use at your own risk**

## License

This project is for educational purposes only. See LICENSE file for details.

## Support

For issues and questions, please open an issue on GitHub.
