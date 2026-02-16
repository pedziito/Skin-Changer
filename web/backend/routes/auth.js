const express = require('express');
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const { getDatabase } = require('../database');

const router = express.Router();
const JWT_SECRET = process.env.JWT_SECRET || 'cs2-skin-changer-secret-key-change-in-production';

// Register new user
router.post('/register', async (req, res) => {
  const { username, email, password, license_key } = req.body;

  // Validation
  if (!username || !email || !password || !license_key) {
    return res.status(400).json({ error: 'All fields including license key are required' });
  }

  if (password.length < 6) {
    return res.status(400).json({ error: 'Password must be at least 6 characters' });
  }

  try {
    const db = getDatabase();
    
    // Check if license key is valid
    db.get('SELECT * FROM license_keys WHERE key = ? AND is_used = 0', [license_key], async (err, licenseRow) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }
      
      if (!licenseRow) {
        return res.status(400).json({ error: 'Invalid or already used license key' });
      }
      
      // Check if license is expired
      if (licenseRow.expires_at) {
        const expiryDate = new Date(licenseRow.expires_at);
        if (expiryDate < new Date()) {
          return res.status(400).json({ error: 'License key has expired' });
        }
      }
    
      // Check if user already exists
      db.get('SELECT id FROM users WHERE username = ? OR email = ?', [username, email], async (err, row) => {
        if (err) {
          return res.status(500).json({ error: 'Database error' });
        }
        
        if (row) {
          return res.status(400).json({ error: 'Username or email already exists' });
        }

        // Hash password
        const hashedPassword = await bcrypt.hash(password, 10);

        // Insert user with license key
        db.run('INSERT INTO users (username, email, password, license_key) VALUES (?, ?, ?, ?)', 
          [username, email, hashedPassword, license_key], 
          function(err) {
            if (err) {
              return res.status(500).json({ error: 'Failed to create user' });
            }

            const userId = this.lastID;
            
            // Mark license key as used
            db.run(
              'UPDATE license_keys SET is_used = 1, used_by = ?, used_at = CURRENT_TIMESTAMP WHERE key = ?',
              [userId, license_key],
              (err) => {
                if (err) {
                  console.error('Failed to update license key:', err);
                }
              }
            );

            // Generate JWT token
            const token = jwt.sign({ userId, username }, JWT_SECRET, { expiresIn: '7d' });

            res.status(201).json({
              message: 'User registered successfully',
              token,
              user: {
                id: userId,
                username,
                email
              }
            });
          }
        );
      });
    });
  } catch (error) {
    res.status(500).json({ error: 'Server error' });
  }
});
  } catch (error) {
    res.status(500).json({ error: 'Server error' });
  }
});

// Login
router.post('/login', (req, res) => {
  const { username, password } = req.body;

  if (!username || !password) {
    return res.status(400).json({ error: 'Username and password are required' });
  }

  const db = getDatabase();

  db.get('SELECT * FROM users WHERE username = ?', [username], async (err, user) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }

    if (!user) {
      return res.status(401).json({ error: 'Invalid credentials' });
    }

    // Check if user is active
    if (user.is_active !== 1) {
      return res.status(403).json({ error: 'Account has been deactivated. Contact administrator.' });
    }

    // Verify password
    const isValidPassword = await bcrypt.compare(password, user.password);
    
    if (!isValidPassword) {
      return res.status(401).json({ error: 'Invalid credentials' });
    }

    // Update last login
    db.run('UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = ?', [user.id]);

    // Generate JWT token
    const token = jwt.sign({ userId: user.id, username: user.username }, JWT_SECRET, { expiresIn: '7d' });

    res.json({
      message: 'Login successful',
      token,
      user: {
        id: user.id,
        username: user.username,
        email: user.email,
        is_admin: user.is_admin === 1
      }
    });
  });
});

// Verify token
router.get('/verify', (req, res) => {
  const token = req.headers.authorization?.split(' ')[1];

  if (!token) {
    return res.status(401).json({ error: 'No token provided' });
  }

  try {
    const decoded = jwt.verify(token, JWT_SECRET);
    res.json({ valid: true, userId: decoded.userId, username: decoded.username });
  } catch (error) {
    res.status(401).json({ error: 'Invalid token' });
  }
});

// Generate API token for .exe client
router.post('/generate-api-token', (req, res) => {
  const token = req.headers.authorization?.split(' ')[1];

  if (!token) {
    return res.status(401).json({ error: 'No token provided' });
  }

  try {
    const decoded = jwt.verify(token, JWT_SECRET);
    const db = getDatabase();

    // Generate a unique API token
    const apiToken = jwt.sign({ userId: decoded.userId, type: 'api' }, JWT_SECRET, { expiresIn: '365d' });

    // Store API token in database
    db.run('INSERT INTO api_tokens (user_id, token, expires_at) VALUES (?, ?, datetime("now", "+365 days"))',
      [decoded.userId, apiToken],
      function(err) {
        if (err) {
          return res.status(500).json({ error: 'Failed to generate API token' });
        }

        res.json({
          message: 'API token generated successfully',
          apiToken,
          expiresIn: '365 days'
        });
      }
    );
  } catch (error) {
    res.status(401).json({ error: 'Invalid token' });
  }
});

module.exports = router;
