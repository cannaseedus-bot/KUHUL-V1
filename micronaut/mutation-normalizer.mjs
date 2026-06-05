/**
 * ============================================================================
 * UNIVERSAL MUTATION NORMALIZATION LAYER
 * ============================================================================
 *
 * Every state change becomes a FIELD mutation.
 * Tracks entropy cost, phase legality, and history consistency.
 * No loose state changes allowed.
 */

import { FIELD } from './field.mjs';

/**
 * MutationNormalizer
 * Converts all state changes into FIELD delta objects
 */
export class MutationNormalizer {
  constructor() {
    this.mutation_history = [];
    this.mutation_count = 0;
  }

  /**
   * Normalize any state change into FIELD mutation format
   * Returns { field_id, delta, entropy_cost, phase, timestamp, legal: boolean }
   */
  normalizeMutation(old_state, new_state, context = {}) {
    const delta = this.computeDelta(old_state, new_state);
    const entropy_cost = this.computeEntropyCost(delta);

    const mutation = new FIELD({
      type: 'mutation',
      bindings: {
        source: context.source || 'unknown',
        phase: context.phase || 'Pop',
        operator: context.operator || 'update',
      },
      state: { old: old_state, new: new_state },
      delta,
      entropy: entropy_cost,
    });

    // Validate mutation legality
    const validation = mutation.validate();
    mutation.legal = validation.valid;

    this.mutation_history.push(mutation);
    this.mutation_count++;

    return mutation;
  }

  /**
   * Compute delta between two states
   * Format: { field: { old, new }, ... }
   */
  computeDelta(old_state, new_state) {
    const delta = {};

    // Find changed keys
    const all_keys = new Set([
      ...Object.keys(old_state || {}),
      ...Object.keys(new_state || {}),
    ]);

    for (const key of all_keys) {
      const old_val = old_state?.[key];
      const new_val = new_state?.[key];

      if (JSON.stringify(old_val) !== JSON.stringify(new_val)) {
        delta[key] = { old: old_val, new: new_val };
      }
    }

    return delta;
  }

  /**
   * Compute entropy cost of mutation
   * Reflects how much disorder is introduced by the change
   */
  computeEntropyCost(delta) {
    if (!delta || Object.keys(delta).length === 0) {
      return 0; // No-op mutation
    }

    const changed_fields = Object.keys(delta).length;
    const base_cost = Math.min(changed_fields * 0.05, 0.3); // Up to 0.3

    // Add complexity penalty for deep changes
    let complexity = 0;
    for (const field_delta of Object.values(delta)) {
      const old_size = JSON.stringify(field_delta.old).length;
      const new_size = JSON.stringify(field_delta.new).length;
      const size_ratio = Math.abs(new_size - old_size) / Math.max(old_size, new_size, 1);
      complexity += size_ratio;
    }

    const complexity_penalty = Math.min(complexity / changed_fields, 0.2);

    return Math.min(base_cost + complexity_penalty, 1);
  }

  /**
   * Get mutation history (filtered)
   */
  getMutationHistory(filter = {}) {
    let history = this.mutation_history;

    if (filter.phase) {
      history = history.filter((m) => m.bindings.phase === filter.phase);
    }

    if (filter.legal_only) {
      history = history.filter((m) => m.legal);
    }

    if (filter.source) {
      history = history.filter((m) => m.bindings.source === filter.source);
    }

    return history;
  }

  /**
   * Export statistics
   */
  getStats() {
    const legal_count = this.mutation_history.filter((m) => m.legal).length;

    return {
      total_mutations: this.mutation_count,
      legal_mutations: legal_count,
      illegal_mutations: this.mutation_count - legal_count,
      legality_rate: this.mutation_count > 0 ? legal_count / this.mutation_count : 1,
      avg_entropy_cost: this.mutation_history.length > 0
        ? this.mutation_history.reduce((sum, m) => sum + m.entropy, 0) / this.mutation_history.length
        : 0,
    };
  }
}

/**
 * StatefulFieldStore
 * Maintains mutable state as FIELD objects with history tracking
 */
export class StatefulFieldStore {
  constructor() {
    this.fields = new Map(); // field_id → FIELD
    this.normalizer = new MutationNormalizer();
  }

  /**
   * Create or update a field with state change
   * Automatically creates mutation FIELD
   */
  updateField(field_id, new_state, context = {}) {
    const existing_field = this.fields.get(field_id);
    const old_state = existing_field?.state || {};

    // Normalize mutation
    const mutation = this.normalizer.normalizeMutation(old_state, new_state, {
      source: context.source || field_id,
      phase: context.phase || 'Pop',
      ...context,
    });

    if (!mutation.legal) {
      return {
        ok: false,
        reason: 'Mutation failed legality check',
        mutation_id: mutation.id,
      };
    }

    // Update or create field
    if (!existing_field) {
      const field = new FIELD({
        id: field_id,
        type: context.type || 'state',
        bindings: context.bindings || {},
        state: new_state,
      });
      field.appendHistory(this.normalizer.mutation_count, context.phase || 'Pop', mutation.entropy);
      this.fields.set(field_id, field);
    } else {
      existing_field.state = new_state;
      existing_field.appendHistory(this.normalizer.mutation_count, context.phase || 'Pop', mutation.entropy);
    }

    return {
      ok: true,
      field_id,
      mutation_id: mutation.id,
      entropy_delta: mutation.entropy,
    };
  }

  /**
   * Get field by ID
   */
  getField(field_id) {
    return this.fields.get(field_id);
  }

  /**
   * Get field state snapshot
   */
  getFieldState(field_id) {
    const field = this.fields.get(field_id);
    return field?.state || null;
  }

  /**
   * Get field history
   */
  getFieldHistory(field_id) {
    const field = this.fields.get(field_id);
    return field?.history || [];
  }

  /**
   * List all fields
   */
  getAllFields() {
    return Array.from(this.fields.values());
  }

  /**
   * Export state snapshot
   */
  exportSnapshot() {
    return {
      field_count: this.fields.size,
      fields: Array.from(this.fields.values()).map((f) => ({
        id: f.id,
        type: f.type,
        state: f.state,
        entropy: f.entropy,
        history_entries: f.history.length,
      })),
      mutation_stats: this.normalizer.getStats(),
    };
  }
}

/**
 * EntropyCostAccounting
 * Tracks cumulative entropy introduced by mutations
 */
export class EntropyCostAccounting {
  constructor() {
    this.entropy_ledger = []; // { timestamp, source, cost, reason }
    this.total_entropy = 0;
    this.entropy_budget = 1.0; // Maximum allowed cumulative entropy
  }

  /**
   * Log entropy cost
   */
  addEntropyCost(cost, source = 'mutation', reason = '') {
    if (cost < 0 || cost > 1) {
      return { ok: false, reason: 'Entropy cost must be in [0, 1]' };
    }

    const remaining = this.entropy_budget - cost;

    if (remaining < 0) {
      return {
        ok: false,
        reason: `Entropy budget exceeded: ${cost} > ${this.entropy_budget}`,
        current_entropy: this.total_entropy,
        budget_remaining: this.entropy_budget,
      };
    }

    this.entropy_ledger.push({
      timestamp: new Date().toISOString(),
      source,
      cost,
      reason,
    });

    this.total_entropy += cost;
    this.entropy_budget -= cost;

    return {
      ok: true,
      entropy_added: cost,
      total_entropy: this.total_entropy,
      budget_remaining: this.entropy_budget,
    };
  }

  /**
   * Check if entropy budget allows operation
   */
  canAllocate(required_entropy) {
    return this.entropy_budget >= required_entropy;
  }

  /**
   * Get entropy accounting report
   */
  getReport() {
    return {
      total_entropy: this.total_entropy,
      entropy_budget: this.entropy_budget,
      budget_usage_percent: (this.total_entropy / 1.0) * 100,
      ledger_size: this.entropy_ledger.length,
      entries_by_source: this.groupBySource(),
    };
  }

  /**
   * Group ledger entries by source
   */
  groupBySource() {
    const grouped = {};

    for (const entry of this.entropy_ledger) {
      if (!grouped[entry.source]) {
        grouped[entry.source] = { count: 0, total_cost: 0 };
      }
      grouped[entry.source].count++;
      grouped[entry.source].total_cost += entry.cost;
    }

    return grouped;
  }

  /**
   * Export state
   */
  exportState() {
    return this.getReport();
  }
}

