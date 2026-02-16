const sqlite3 = require('sqlite3').verbose();
const path = require('path');

const DB_PATH = path.join(__dirname, 'skinchanger.db');

let db = null;

function getDatabase() {
  if (!db) {
    db = new sqlite3.Database(DB_PATH, (err) => {
      if (err) {
        console.error('Error opening database:', err);
      } else {
        console.log('✅ Connected to SQLite database');
      }
    });
  }
  return db;
}

function initDatabase() {
  return new Promise((resolve, reject) => {
    const db = getDatabase();

    db.serialize(() => {
      // Users table
      db.run(`
        CREATE TABLE IF NOT EXISTS users (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          username TEXT UNIQUE NOT NULL,
          email TEXT UNIQUE NOT NULL,
          password TEXT NOT NULL,
          license_key TEXT,
          is_admin INTEGER DEFAULT 0,
          is_active INTEGER DEFAULT 1,
          created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
          last_login DATETIME,
          FOREIGN KEY (license_key) REFERENCES license_keys(key)
        )
      `, (err) => {
        if (err) reject(err);
      });

      // Skin configurations table
      db.run(`
        CREATE TABLE IF NOT EXISTS skin_configs (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          user_id INTEGER NOT NULL,
          weapon_category TEXT NOT NULL,
          weapon_name TEXT NOT NULL,
          weapon_id INTEGER NOT NULL,
          skin_name TEXT NOT NULL,
          paint_kit INTEGER NOT NULL,
          created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
          updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
          FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        )
      `, (err) => {
        if (err) reject(err);
      });

      // API tokens table for .exe authentication
      db.run(`
        CREATE TABLE IF NOT EXISTS api_tokens (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          user_id INTEGER NOT NULL,
          token TEXT UNIQUE NOT NULL,
          created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
          expires_at DATETIME,
          last_used DATETIME,
          FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        )
      `, (err) => {
        if (err) reject(err);
      });

      // License keys table
      db.run(`
        CREATE TABLE IF NOT EXISTS license_keys (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          key TEXT UNIQUE NOT NULL,
          is_used INTEGER DEFAULT 0,
          used_by INTEGER,
          created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
          used_at DATETIME,
          expires_at DATETIME,
          notes TEXT,
          FOREIGN KEY (used_by) REFERENCES users(id) ON DELETE SET NULL
        )
      `, (err) => {
        if (err) {
          reject(err);
        } else {
          console.log('✅ Database tables initialized');
          
          // Create default admin account if it doesn't exist
          db.get('SELECT id FROM users WHERE username = ?', ['admin'], (err, row) => {
            if (!row) {
              const bcrypt = require('bcryptjs');
              const adminPassword = bcrypt.hashSync('admin123', 10);
              db.run(
                'INSERT INTO users (username, email, password, is_admin) VALUES (?, ?, ?, ?)',
                ['admin', 'admin@skinchanger.local', adminPassword, 1],
                (err) => {
                  if (!err) {
                    console.log('✅ Default admin account created (username: admin, password: admin123)');
                  }
                }
              );
            }
          });
          
          resolve();
        }
      });
    });
  });
}

module.exports = {
  getDatabase,
  initDatabase
};
