const assert = require('assert');
const app = require('./src/index');
const http = require('http');

async function runTests() {
  console.log('=== Starting Server & Bridge Integration Tests ===');

  const server = http.createServer(app);
  await new Promise((resolve) => server.listen(0, resolve));
  const port = server.address().port;
  const baseUrl = `http://localhost:${port}`;

  try {
    // 1. Health check
    const healthRes = await fetch(`${baseUrl}/api/health`);
    const healthJson = await healthRes.json();
    assert.strictEqual(healthRes.status, 200, 'Health check status should be 200');
    assert.strictEqual(healthJson.status, 'ok', 'Health status should be ok');
    console.log('✓ Health check passed');

    // 2. Generate default architecture
    const genRes = await fetch(`${baseUrl}/api/architecture/generate`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ num_p_cores: 8, num_e_cores: 8 })
    });
    const archJson = await genRes.json();
    assert.strictEqual(genRes.status, 200, 'Generate arch status should be 200');
    assert.strictEqual(archJson.config.num_p_cores, 8, 'Should have 8 P-cores');
    assert(archJson.components.length > 40, 'Should have >40 components');
    console.log(`✓ Architecture generation passed (${archJson.components.length} components, ${archJson.nets.length} nets)`);

    // 3. Validate placement
    const valRes = await fetch(`${baseUrl}/api/architecture/validate-placement`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(archJson)
    });
    const valJson = await valRes.json();
    assert.strictEqual(valRes.status, 200, 'Validate placement status should be 200');
    assert.strictEqual(valJson.success, true, 'Placement should be valid');
    console.log('✓ Placement validation passed');

    // 4. Dijkstra routing
    const dRes = await fetch(`${baseUrl}/api/route/dijkstra`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ architecture: archJson })
    });
    const dJson = await dRes.json();
    assert.strictEqual(dRes.status, 200, 'Dijkstra route status should be 200');
    assert.strictEqual(dJson.algorithm, 'Dijkstra_Baseline');
    assert(dJson.summary.routed_nets > 0, 'Dijkstra should route nets');
    console.log(`✓ Dijkstra routing passed (${dJson.summary.routed_nets}/${dJson.summary.total_nets} routed, length=${dJson.summary.total_wirelength.toFixed(1)})`);

    // 5. Invalid request handling
    const invRes = await fetch(`${baseUrl}/api/architecture/generate`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ num_p_cores: -5 })
    });
    const invJson = await invRes.json();
    assert.strictEqual(invRes.status, 400, 'Invalid request should return 400');
    assert(invJson.error.includes('P-core count'), 'Should report specific validation error');
    console.log('✓ Error recovery & input validation passed');

    console.log('\nALL SERVER INTEGRATION TESTS PASSED!');
  } finally {
    server.close();
  }
}

runTests().catch((err) => {
  console.error('Test failed:', err);
  process.exit(1);
});
