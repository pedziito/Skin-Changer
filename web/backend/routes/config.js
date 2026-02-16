const express = require('express');
const jwt = require('jsonwebtoken');
const { getDatabase } = require('../database');

const router = express.Router();
const JWT_SECRET = process.env.JWT_SECRET || 'cs2-skin-changer-secret-key-change-in-production';

// Middleware to verify JWT token
function authenticateToken(req, res, next) {
  const authHeader = req.headers['authorization'];
  const token = authHeader && authHeader.split(' ')[1];

  if (!token) {
    return res.status(401).json({ error: 'Access token required' });
  }

  jwt.verify(token, JWT_SECRET, (err, user) => {
    if (err) {
      return res.status(403).json({ error: 'Invalid or expired token' });
    }
    req.user = user;
    next();
  });
}

// Get all skin configurations for user
router.get('/', authenticateToken, (req, res) => {
  const db = getDatabase();
  
  db.all(
    'SELECT * FROM skin_configs WHERE user_id = ? ORDER BY updated_at DESC',
    [req.user.userId],
    (err, configs) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }
      res.json({ configs });
    }
  );
});

// Get specific skin config by ID
router.get('/:id', authenticateToken, (req, res) => {
  const db = getDatabase();
  
  db.get(
    'SELECT * FROM skin_configs WHERE id = ? AND user_id = ?',
    [req.params.id, req.user.userId],
    (err, config) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }
      
      if (!config) {
        return res.status(404).json({ error: 'Configuration not found' });
      }
      
      res.json({ config });
    }
  );
});

// Create or update skin configuration
router.post('/', authenticateToken, (req, res) => {
  const { weapon_category, weapon_name, weapon_id, skin_name, paint_kit } = req.body;

  if (!weapon_category || !weapon_name || !weapon_id || !skin_name || !paint_kit) {
    return res.status(400).json({ error: 'All fields are required' });
  }

  const db = getDatabase();

  // Check if config already exists for this weapon
  db.get(
    'SELECT id FROM skin_configs WHERE user_id = ? AND weapon_id = ?',
    [req.user.userId, weapon_id],
    (err, existing) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }

      if (existing) {
        // Update existing config
        db.run(
          `UPDATE skin_configs 
           SET weapon_category = ?, weapon_name = ?, skin_name = ?, paint_kit = ?, updated_at = CURRENT_TIMESTAMP
           WHERE id = ? AND user_id = ?`,
          [weapon_category, weapon_name, skin_name, paint_kit, existing.id, req.user.userId],
          function(err) {
            if (err) {
              return res.status(500).json({ error: 'Failed to update configuration' });
            }
            res.json({ 
              message: 'Configuration updated successfully',
              id: existing.id
            });
          }
        );
      } else {
        // Create new config
        db.run(
          `INSERT INTO skin_configs (user_id, weapon_category, weapon_name, weapon_id, skin_name, paint_kit)
           VALUES (?, ?, ?, ?, ?, ?)`,
          [req.user.userId, weapon_category, weapon_name, weapon_id, skin_name, paint_kit],
          function(err) {
            if (err) {
              return res.status(500).json({ error: 'Failed to create configuration' });
            }
            res.status(201).json({
              message: 'Configuration created successfully',
              id: this.lastID
            });
          }
        );
      }
    }
  );
});

// Delete skin configuration
router.delete('/:id', authenticateToken, (req, res) => {
  const db = getDatabase();
  
  db.run(
    'DELETE FROM skin_configs WHERE id = ? AND user_id = ?',
    [req.params.id, req.user.userId],
    function(err) {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }
      
      if (this.changes === 0) {
        return res.status(404).json({ error: 'Configuration not found' });
      }
      
      res.json({ message: 'Configuration deleted successfully' });
    }
  );
});

// Reset all configurations
router.delete('/', authenticateToken, (req, res) => {
  const db = getDatabase();
  
  db.run(
    'DELETE FROM skin_configs WHERE user_id = ?',
    [req.user.userId],
    function(err) {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }
      
      res.json({ 
        message: 'All configurations deleted successfully',
        deletedCount: this.changes
      });
    }
  );
});

module.exports = router;
