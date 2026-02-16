const express = require('express');
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const { getDatabase } = require('../database');

const router = express.Router();
const JWT_SECRET = process.env.JWT_SECRET || 'cs2-skin-changer-secret-key-change-in-production';

// Register new user
router.post('/register', async (req, res) => {
  const { username, email, password } = req.body;

  // Validation
  if (!username || !email || !password) {
    return res.status(400).json({ error: 'All fields are required' });
  }

  if (password.length < 6) {
    return res.status(400).json({ error: 'Password must be at least 6 characters' });
  }

  try {
    const db = getDatabase();
    
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

      // Insert user
      db.run('INSERT INTO users (username, email, password) VALUES (?, ?, ?)', 
        [username, email, hashedPassword], 
        function(err) {
          if (err) {
            return res.status(500).json({ error: 'Failed to create user' });
          }

          // Generate JWT token
          const token = jwt.sign({ userId: this.lastID, username }, JWT_SECRET, { expiresIn: '7d' });

          res.status(201).json({
            message: 'User registered successfully',
            token,
            user: {
              id: this.lastID,
              username,
              email
            }
          });
        }
      );
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
        email: user.email
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
