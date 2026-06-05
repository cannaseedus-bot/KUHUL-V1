#!/usr/bin/env node
/**
 * test-geometry-shard.mjs
 * 
 * Verify tiny.x DirectX mesh geometry shard integration
 * 
 * Tests:
 *   - Manifest registration
 *   - Shard lookup and discovery
 *   - Residency management
 *   - Projection layer binding
 *   - FIELD → geometry mapping
 */

import {
  GeometryWeightDescriptor,
  GeometryWeightRegistry,
  ProjectionLayer,
  createTinyXManifest,
} from './geometry-shard-manifest.mjs';
import { FIELD } from './field.mjs';

let test_count = 0;
let pass_count = 0;

function test(name, fn) {
  test_count++;
  try {
    fn();
    console.log(`✓ TEST ${test_count}: ${name}`);
    pass_count++;
  } catch (e) {
    console.log(`✗ TEST ${test_count}: ${name}`);
    console.log(`  Error: ${e.message}`);
  }
}

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

console.log('╔════════════════════════════════════════════════════════════════╗');
console.log('║  GEOMETRY-WEIGHT INTEGRATION TEST                             ║');
console.log('║  Verify: tiny.x DirectX Mesh → Geometric Parameter Binding    ║');
console.log('╚════════════════════════════════════════════════════════════════╝\n');

// ─────────────────────────────────────────────────────────────────────────────
// TEST 1: Manifest Creation & Validation
// ─────────────────────────────────────────────────────────────────────────────

console.log('TEST GROUP 1: Geometry Weight Manifest\n');

test('Create valid geometry weight descriptor', () => {
  const manifest = createTinyXManifest();

  const descriptor = new GeometryWeightDescriptor(manifest);
  descriptor.validate();

  assert(descriptor.kind === 'geometry-weight', 'Kind should be geometry-weight');
  assert(descriptor.name === 'tiny_x_weight', 'Name should be tiny_x_weight');
  assert(descriptor.phase_key === 'tiny_geometry', 'Phase key should be tiny_geometry');
  assert(descriptor.role === 'projection-prior', 'Role should be projection-prior');
  assert(descriptor.format === 'DirectX_x', 'Format should be DirectX_x');
  assert(descriptor.residency_hint === 'cold', 'Residency should be cold');
  assert(descriptor.authority === 'non-kernel', 'Authority should be non-kernel');
});

test('Validation rejects invalid kind', () => {
  try {
    const bad = new GeometryWeightDescriptor({
      kind: 'bad_kind',
      name: 'test',
      phase_key: 'test',
      role: 'projection-prior',
      path: '/test',
      format: 'DirectX_x',
      residency_hint: 'cold',
      authority: 'non-kernel',
    });
    bad.validate();
    throw new Error('Should have thrown');
  } catch (e) {
    assert(e.message.includes('Invalid kind'), `Expected invalid kind error`);
  }
});

test('Validation rejects invalid role', () => {
  try {
    const bad = new GeometryWeightDescriptor({
      kind: 'geometry-weight',
      name: 'test',
      phase_key: 'test',
      role: 'bad_role',
      path: '/test',
      format: 'DirectX_x',
      residency_hint: 'cold',
      authority: 'non-kernel',
    });
    bad.validate();
    throw new Error('Should have thrown');
  } catch (e) {
    assert(e.message.includes('Invalid role'), `Expected invalid role error`);
  }
});

test('Validation rejects invalid residency', () => {
  try {
    const bad = new GeometryWeightDescriptor({
      kind: 'geometry-weight',
      name: 'test',
      phase_key: 'test',
      role: 'projection-prior',
      path: '/test',
      format: 'DirectX_x',
      residency_hint: 'bad_residency',
      authority: 'non-kernel',
    });
    bad.validate();
    throw new Error('Should have thrown');
  } catch (e) {
    assert(e.message.includes('Invalid residency'), `Expected invalid residency error`);
  }
});

// ─────────────────────────────────────────────────────────────────────────────
// TEST 2: Registry Management
// ─────────────────────────────────────────────────────────────────────────────

console.log('\nTEST GROUP 2: Geometry Weight Registry\n');

test('Register geometry weight', () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();

  const result = registry.register(manifest);

  assert(result.success === true, 'Registration should succeed');
  assert(result.phase_key === 'tiny_geometry', 'Phase key should be returned');
  assert(result.role === 'projection-prior', 'Role should be returned');
  assert(result.authority === 'non-kernel', 'Authority should be non-kernel');
});

test('Lookup registered weight', () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();

  registry.register(manifest);

  const weight = registry.lookup('tiny_geometry');
  assert(weight !== null, 'Should find weight');
  assert(weight.name === 'tiny_x_weight', 'Weight name should match');
});

test('Lookup non-existent weight returns null', () => {
  const registry = new GeometryWeightRegistry();

  const weight = registry.lookup('nonexistent');
  assert(weight === null, 'Should return null for nonexistent weight');
});

test('Get weights by role', () => {
  const registry = new GeometryWeightRegistry();

  registry.register({
    kind: 'geometry-weight',
    name: 'weight1',
    phase_key: 'geom1',
    role: 'projection-prior',
    path: '/path1',
    format: 'DirectX_x',
    residency_hint: 'cold',
    authority: 'non-kernel',
  });

  registry.register({
    kind: 'geometry-weight',
    name: 'weight2',
    phase_key: 'geom2',
    role: 'routing-bias',
    path: '/path2',
    format: 'obj',
    residency_hint: 'warm',
    authority: 'non-kernel',
  });

  const priors = registry.getByRole('projection-prior');
  assert(priors.length === 1, `Should find 1 projection-prior, got ${priors.length}`);
  assert(priors[0].name === 'weight1', 'Should be weight1');

  const biases = registry.getByRole('routing-bias');
  assert(biases.length === 1, `Should find 1 routing-bias`);
});

test('List all registered weights', () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();

  registry.register(manifest);
  registry.register({
    kind: 'geometry-weight',
    name: 'reference',
    phase_key: 'ref_geom',
    role: 'shape-prior',
    path: '/ref',
    format: 'glb',
    residency_hint: 'paged',
    authority: 'non-kernel',
  });

  const all = registry.listAll();
  assert(all.length === 2, `Should have 2 weights, got ${all.length}`);
  assert(all[0].loaded === false, 'Weights should not be loaded initially');
  assert(all[0].authority === 'non-kernel', 'Authority should be non-kernel');
});

// ─────────────────────────────────────────────────────────────────────────────
// TEST 3: Residency Management
// ─────────────────────────────────────────────────────────────────────────────

console.log('\nTEST GROUP 3: Residency Management\n');

test('Load weight changes residency state', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();

  registry.register(manifest);

  const weight_before = registry.lookup('tiny_geometry');
  assert(weight_before.loaded === false, 'Should not be loaded initially');

  await registry.load('tiny_geometry');

  const weight_after = registry.lookup('tiny_geometry');
  assert(weight_after.loaded === true, 'Should be loaded after load()');
  assert(weight_after.asset !== null, 'Asset should be populated');
  assert(weight_after.loading_time > 0, 'Loading time should be recorded');
});

test('Unload weight releases memory', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();

  registry.register(manifest);
  await registry.load('tiny_geometry');

  let weight = registry.lookup('tiny_geometry');
  assert(weight.loaded === true, 'Should be loaded');

  registry.unload('tiny_geometry');

  weight = registry.lookup('tiny_geometry');
  assert(weight.loaded === false, 'Should be unloaded');
  assert(weight.asset === null, 'Asset should be cleared');
});

test('Get statistics', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();

  registry.register(manifest);
  await registry.load('tiny_geometry');

  const stats = registry.getStats();

  assert(stats.total_weights === 1, 'Should have 1 weight');
  assert(stats.loaded_count === 1, 'Should have 1 loaded');
  assert(stats.total_memory_bytes > 0, 'Should have memory');
  assert(stats.by_residency.cold === 1, 'Should have 1 cold weight');
  assert(stats.by_role['projection-prior'] === 1, 'Should have 1 projection-prior');
});

// ─────────────────────────────────────────────────────────────────────────────
// TEST 4: Projection Layer
// ─────────────────────────────────────────────────────────────────────────────

console.log('\nTEST GROUP 4: Projection Layer\n');

test('Project FIELD to geometry weight', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();
  registry.register(manifest);
  await registry.load('tiny_geometry');

  const projection_layer = new ProjectionLayer(registry);

  const field = new FIELD({
    type: 'semantic',
    state: { query: 'test projection' },
    entropy: 0.5,
  });

  const projection = projection_layer.project(field, 'tiny_geometry');

  assert(projection.field_id === field.id, 'Projection should reference FIELD');
  assert(projection.shard_key === 'tiny_geometry', 'Should reference weight');
  assert(projection.geometry.format === 'DirectX_x', 'Format should be DirectX_x');
  assert(projection.geometry.entropy_rotation !== undefined, 'Should compute entropy rotation');
  assert(
    projection.visual_hints.opacity >= 0.5 && projection.visual_hints.opacity <= 1.0,
    'Opacity should be [0.5, 1.0]'
  );
});

test('Phase-to-frame mapping', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();
  registry.register(manifest);
  await registry.load('tiny_geometry');

  const projection_layer = new ProjectionLayer(registry);

  const pop_field = new FIELD({
    type: 'semantic',
    state: { query: 'pop' },
    entropy: 0.0,
  });
  pop_field.phase = 'Pop';

  const pop_proj = projection_layer.project(pop_field, 'tiny_geometry');
  assert(pop_proj.geometry.phase_frame === 0, 'Pop phase should map to frame 0');

  const xul_field = new FIELD({
    type: 'semantic',
    state: { query: 'xul' },
    entropy: 0.0,
  });
  xul_field.phase = 'Xul';

  const xul_proj = projection_layer.project(xul_field, 'tiny_geometry');
  assert(xul_proj.geometry.phase_frame > pop_proj.geometry.phase_frame, 'Xul should map to later frame');
});

test('Retrieve projection by FIELD ID', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();
  registry.register(manifest);
  await registry.load('tiny_geometry');

  const projection_layer = new ProjectionLayer(registry);

  const field = new FIELD({
    type: 'semantic',
    state: { query: 'test' },
    entropy: 0.3,
  });

  projection_layer.project(field, 'tiny_geometry');

  const retrieved = projection_layer.getProjection(field.id);
  assert(retrieved !== null, 'Should retrieve projection');
  assert(retrieved.field_id === field.id, 'Should be same FIELD');
});

test('Prune old projections', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();
  registry.register(manifest);
  await registry.load('tiny_geometry');

  const projection_layer = new ProjectionLayer(registry);

  const field1 = new FIELD({ type: 'semantic', state: { q: '1' }, entropy: 0.0 });
  const field2 = new FIELD({ type: 'semantic', state: { q: '2' }, entropy: 0.0 });

  projection_layer.project(field1, 'tiny_geometry');
  projection_layer.project(field2, 'tiny_geometry');

  assert(projection_layer.projections.size === 2, 'Should have 2 projections');

  // Prune with very short max_age (everything older than 1ms)
  await new Promise(resolve => setTimeout(resolve, 10));
  projection_layer.prune(5); // max_age = 5ms

  assert(projection_layer.projections.size === 0, 'All projections should be pruned');
});

// ─────────────────────────────────────────────────────────────────────────────
// TEST 5: Integration with FIELD
// ─────────────────────────────────────────────────────────────────────────────

console.log('\nTEST GROUP 5: FIELD Integration\n');

test('Geometry weight works with valid FIELD', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();
  registry.register(manifest);
  await registry.load('tiny_geometry');

  const projection_layer = new ProjectionLayer(registry);

  const field = new FIELD({
    type: 'semantic',
    state: { input: 'test data' },
    entropy: 0.45,
  });

  assert(field.id !== null, 'FIELD should have ID');

  const projection = projection_layer.project(field, 'tiny_geometry');
  assert(projection !== null, 'Should create projection');
  assert(projection.field_id === field.id, 'Projection should reference FIELD ID');
});

test('Multiple FIELDs map to same geometry weight', async () => {
  const registry = new GeometryWeightRegistry();
  const manifest = createTinyXManifest();
  registry.register(manifest);
  await registry.load('tiny_geometry');

  const projection_layer = new ProjectionLayer(registry);

  const field1 = new FIELD({ type: 'semantic', state: { a: 1 }, entropy: 0.2 });
  const field2 = new FIELD({ type: 'semantic', state: { b: 2 }, entropy: 0.7 });

  const proj1 = projection_layer.project(field1, 'tiny_geometry');
  const proj2 = projection_layer.project(field2, 'tiny_geometry');

  assert(proj1.shard_key === proj2.shard_key, 'Both should map to same weight');
  assert(proj1.field_id !== proj2.field_id, 'But different FIELDs');
  assert(
    Math.abs(proj1.visual_hints.opacity - proj2.visual_hints.opacity) > 0.1,
    'Different entropy should produce different opacity'
  );
});

// ─────────────────────────────────────────────────────────────────────────────
// SUMMARY
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n' + '═'.repeat(68));
console.log(`TEST SUMMARY\n`);
console.log(`Total Tests: ${test_count}`);
console.log(`Passed: ${pass_count}`);
console.log(`Failed: ${test_count - pass_count}`);

if (pass_count === test_count) {
  console.log('\n🎉 ALL TESTS PASSED! Geometry-weight integration verified.\n');
} else {
  console.log(`\n⚠️  ${test_count - pass_count} test(s) failed.\n`);
}

console.log('═'.repeat(68));

process.exit(pass_count === test_count ? 0 : 1);
