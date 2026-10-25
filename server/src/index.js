const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = process.env.PORT || 3001;
const ENGINE_PATH = path.resolve(__dirname, '../../engine/bin/physarum_chip');

app.use(cors());
app.use(express.json({ limit: '50mb' }));

// Helper to invoke C11 standalone engine safely via explicit args and stdio
function invokeEngine(modeFlag, payload = null, timeoutMs = 60000) {
  return new Promise((resolve, reject) => {
    if (!fs.existsSync(ENGINE_PATH)) {
      return reject(new Error(`PhysarumChip C engine binary not found at: ${ENGINE_PATH}. Please run 'make -C engine' first.`));
    }

    const child = spawn(ENGINE_PATH, [modeFlag], {
      stdio: ['pipe', 'pipe', 'pipe']
    });

    let stdoutData = '';
    let stderrData = '';
    let timer = null;

    if (timeoutMs > 0) {
      timer = setTimeout(() => {
        child.kill('SIGKILL');
        reject(new Error(`C Engine execution timed out after ${timeoutMs}ms for mode: ${modeFlag}`));
      }, timeoutMs);
    }

    child.stdout.on('data', (chunk) => {
      stdoutData += chunk.toString();
    });

    child.stderr.on('data', (chunk) => {
      stderrData += chunk.toString();
    });

    child.on('error', (err) => {
      if (timer) clearTimeout(timer);
      reject(err);
    });

    child.on('close', (code) => {
      if (timer) clearTimeout(timer);

      if (code !== 0 && !stdoutData.trim()) {
        return reject(new Error(`Engine exited with code ${code}: ${stderrData.trim() || 'No error details'}`));
      }

      try {
        const json = JSON.parse(stdoutData.trim());
        if (code !== 0 && json.error) {
          return resolve({ success: false, statusCode: json.status_code || code, error: json.error });
        }
        resolve({ success: true, data: json });
      } catch (err) {
        reject(new Error(`Failed to parse C engine JSON output: ${err.message}. Raw output: ${stdoutData.slice(0, 300)}`));
      }
    });

    if (payload !== null && payload !== undefined) {
      const inputStr = typeof payload === 'string' ? payload : JSON.stringify(payload);
      child.stdin.write(inputStr);
    }
    child.stdin.end();
  });
}

// Health check endpoint
app.get('/api/health', async (req, res) => {
  try {
    const versionRes = await invokeEngine('--version', null, 5000);
    res.json({
      status: 'ok',
      bridge: 'Node.js/Express v1.0',
      engine: versionRes.data || { engine: 'PhysarumChip C11' }
    });
  } catch (err) {
    res.status(500).json({ status: 'error', message: err.message });
  }
});

// Architecture generation endpoint
app.post('/api/architecture/generate', async (req, res) => {
  try {
    const config = req.body.config || req.body;
    const result = await invokeEngine('--generate-arch', config);
    if (!result.success) {
      return res.status(400).json(result);
    }
    res.json(result.data);
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

// Placement validation endpoint
app.post('/api/architecture/validate-placement', async (req, res) => {
  try {
    const result = await invokeEngine('--validate-placement', req.body);
    if (!result.success) {
      return res.status(400).json(result);
    }
    res.json(result.data);
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

// Physarum adaptive flow routing endpoint
app.post('/api/route/physarum', async (req, res) => {
  try {
    const result = await invokeEngine('--route-physarum', req.body, 120000);
    if (!result.success) {
      return res.status(400).json(result);
    }
    res.json(result.data);
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

// Dijkstra baseline routing endpoint
app.post('/api/route/dijkstra', async (req, res) => {
  try {
    const result = await invokeEngine('--route-dijkstra', req.body, 60000);
    if (!result.success) {
      return res.status(400).json(result);
    }
    res.json(result.data);
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

// Comparison endpoint
app.post('/api/route/compare', async (req, res) => {
  try {
    const result = await invokeEngine('--compare', req.body, 180000);
    if (!result.success) {
      return res.status(400).json(result);
    }
    res.json(result.data);
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

// Benchmark run endpoint for reproducibility report
app.post('/api/benchmark', async (req, res) => {
  try {
    const gen = await invokeEngine('--generate-arch', req.body.config || {});
    if (!gen.success) return res.status(400).json(gen);

    const payload = {
      architecture: gen.data,
      routing_params: req.body.routing_params || {}
    };

    const comp = await invokeEngine('--compare', payload, 180000);
    if (!comp.success) return res.status(400).json(comp);

    res.json({
      architecture_summary: {
        components: gen.data.components.length,
        nets: gen.data.nets.length,
        die: `${gen.data.config.die_width}x${gen.data.config.die_height}`
      },
      comparison: comp.data
    });
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

// Serve static frontend build if present
const FRONTEND_DIST = path.resolve(__dirname, '../../frontend/dist');
if (fs.existsSync(FRONTEND_DIST)) {
  app.use(express.static(FRONTEND_DIST));
  app.get('*', (req, res, next) => {
    if (req.path.startsWith('/api')) return next();
    res.sendFile(path.join(FRONTEND_DIST, 'index.html'));
  });
}

if (require.main === module) {
  app.listen(PORT, () => {
    console.log(`PhysarumChip Express Bridge running on http://localhost:${PORT}`);
  });
}

module.exports = app;
