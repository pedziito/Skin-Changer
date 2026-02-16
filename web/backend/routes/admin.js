const express = require('express');
const crypto = require('crypto');
const { getDatabase } = require('../database');
const { requireAdmin } = require('../middleware/adminAuth');

const router = express.Router();

// All routes require admin authentication
router.use(requireAdmin);

// Get all users
router.get('/users', (req, res) => {
  const db = getDatabase();
  
  db.all(`
    SELECT u.id, u.username, u.email, u.license_key, u.is_admin, u.is_active, 
           u.created_at, u.last_login, lk.expires_at as license_expires
    FROM users u
    LEFT JOIN license_keys lk ON u.license_key = lk.key
    ORDER BY u.created_at DESC
  `, (err, users) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }
    res.json({ users });
  });
});

// Get user by ID
router.get('/users/:id', (req, res) => {
  const db = getDatabase();
  
  db.get(`
    SELECT u.id, u.username, u.email, u.license_key, u.is_admin, u.is_active,
           u.created_at, u.last_login, lk.expires_at as license_expires
    FROM users u
    LEFT JOIN license_keys lk ON u.license_key = lk.key
    WHERE u.id = ?
  `, [req.params.id], (err, user) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }
    
    if (!user) {
      return res.status(404).json({ error: 'User not found' });
    }
    
    res.json({ user });
  });
});

// Update user status (activate/deactivate)
router.patch('/users/:id/status', (req, res) => {
  const { is_active } = req.body;
  const db = getDatabase();
  
  db.run(
    'UPDATE users SET is_active = ? WHERE id = ?',
    [is_active ? 1 : 0, req.params.id],
    function(err) {
      if (err) {
        return res.status(500).json({ error: 'Failed to update user status' });
      }
      
      if (this.changes === 0) {
        return res.status(404).json({ error: 'User not found' });
      }
      
      res.json({ message: 'User status updated successfully' });
    }
  );
});

// Assign license key to user
router.post('/users/:id/assign-license', (req, res) => {
  const { license_key } = req.body;
  const db = getDatabase();
  
  // First check if license key exists and is not used
  db.get(
    'SELECT * FROM license_keys WHERE key = ? AND is_used = 0',
    [license_key],
    (err, licenseRow) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }
      
      if (!licenseRow) {
        return res.status(400).json({ error: 'Invalid or already used license key' });
      }
      
      // Update user with license key
      db.run(
        'UPDATE users SET license_key = ? WHERE id = ?',
        [license_key, req.params.id],
        function(err) {
          if (err) {
            return res.status(500).json({ error: 'Failed to assign license key' });
          }
          
          // Mark license as used
          db.run(
            'UPDATE license_keys SET is_used = 1, used_by = ?, used_at = CURRENT_TIMESTAMP WHERE key = ?',
            [req.params.id, license_key],
            (err) => {
              if (err) {
                return res.status(500).json({ error: 'Failed to update license key' });
              }
              
              res.json({ message: 'License key assigned successfully' });
            }
          );
        }
      );
    }
  );
});

// Delete user
router.delete('/users/:id', (req, res) => {
  const db = getDatabase();
  
  // Don't allow deleting admin users
  db.get('SELECT is_admin FROM users WHERE id = ?', [req.params.id], (err, user) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }
    
    if (user && user.is_admin === 1) {
      return res.status(403).json({ error: 'Cannot delete admin users' });
    }
    
    db.run('DELETE FROM users WHERE id = ?', [req.params.id], function(err) {
      if (err) {
        return res.status(500).json({ error: 'Failed to delete user' });
      }
      
      if (this.changes === 0) {
        return res.status(404).json({ error: 'User not found' });
      }
      
      res.json({ message: 'User deleted successfully' });
    });
  });
});

// Get all license keys
router.get('/licenses', (req, res) => {
  const db = getDatabase();
  
  db.all(`
    SELECT lk.*, u.username as used_by_username
    FROM license_keys lk
    LEFT JOIN users u ON lk.used_by = u.id
    ORDER BY lk.created_at DESC
  `, (err, licenses) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }
    res.json({ licenses });
  });
});

// Generate new license keys
router.post('/licenses/generate', (req, res) => {
  const { count = 1, expires_days, notes } = req.body;
  const db = getDatabase();
  
  const keys = [];
  let expires_at = null;
  
  if (expires_days) {
    const expiryDate = new Date();
    expiryDate.setDate(expiryDate.getDate() + parseInt(expires_days));
    expires_at = expiryDate.toISOString();
  }
  
  // Generate keys
  for (let i = 0; i < Math.min(count, 100); i++) {
    const key = 'CS2-' + crypto.randomBytes(12).toString('hex').toUpperCase();
    keys.push(key);
    
    db.run(
      'INSERT INTO license_keys (key, expires_at, notes) VALUES (?, ?, ?)',
      [key, expires_at, notes || null],
      (err) => {
        if (err) {
          console.error('Error inserting license key:', err);
        }
      }
    );
  }
  
  res.json({ 
    message: `${keys.length} license key(s) generated successfully`,
    keys 
  });
});

// Delete license key
router.delete('/licenses/:id', (req, res) => {
  const db = getDatabase();
  
  // Check if license is in use
  db.get('SELECT is_used FROM license_keys WHERE id = ?', [req.params.id], (err, license) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }
    
    if (license && license.is_used === 1) {
      return res.status(403).json({ error: 'Cannot delete license key that is in use' });
    }
    
    db.run('DELETE FROM license_keys WHERE id = ?', [req.params.id], function(err) {
      if (err) {
        return res.status(500).json({ error: 'Failed to delete license key' });
      }
      
      if (this.changes === 0) {
        return res.status(404).json({ error: 'License key not found' });
      }
      
      res.json({ message: 'License key deleted successfully' });
    });
  });
});

// Get dashboard stats
router.get('/stats', (req, res) => {
  const db = getDatabase();
  
  db.get(`
    SELECT 
      (SELECT COUNT(*) FROM users) as total_users,
      (SELECT COUNT(*) FROM users WHERE is_active = 1) as active_users,
      (SELECT COUNT(*) FROM license_keys) as total_licenses,
      (SELECT COUNT(*) FROM license_keys WHERE is_used = 0) as unused_licenses,
      (SELECT COUNT(*) FROM skin_configs) as total_configs
  `, (err, stats) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }
    res.json({ stats });
  });
});

module.exports = router;
