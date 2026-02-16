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
          created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
          last_login DATETIME
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
        if (err) {
          reject(err);
        } else {
          console.log('✅ Database tables initialized');
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
