const jwt = require('jsonwebtoken');
const { getDatabase } = require('../database');

const JWT_SECRET = process.env.JWT_SECRET || 'cs2-skin-changer-secret-key-change-in-production';

// Middleware to verify admin access
function requireAdmin(req, res, next) {
  const authHeader = req.headers['authorization'];
  const token = authHeader && authHeader.split(' ')[1];

  if (!token) {
    return res.status(401).json({ error: 'Access token required' });
  }

  jwt.verify(token, JWT_SECRET, (err, user) => {
    if (err) {
      return res.status(403).json({ error: 'Invalid or expired token' });
    }

    // Check if user is admin
    const db = getDatabase();
    db.get('SELECT is_admin FROM users WHERE id = ?', [user.userId], (err, row) => {
      if (err || !row) {
        return res.status(500).json({ error: 'Database error' });
      }

      if (row.is_admin !== 1) {
        return res.status(403).json({ error: 'Admin access required' });
      }

      req.user = user;
      next();
    });
  });
}

module.exports = {
  requireAdmin
};
