# JSON-RPC Batch Support — `libbitcoin-network` Changes

Review document for the batch-support implementation described in
`libbitcoin-server/docs/json-rpc-batch.md`, Phase 1 (network layer only).

---

## Summary

Seven surgical changes add JSON-RPC 2.0 batch request parsing and sequential
dispatch to the network layer. No existing public API is removed or renamed.
The server layer (`libbitcoin-server`) requires no changes for the Electrum TCP
path; HTTP batch dispatch is deferred to Phase 3.

---

## Changed Files

### 1. `include/bitcoin/network/messages/rpc/model.hpp`

**Change:** Added `batch_t` type alias after `request_t`.

```cpp
using batch_t = std::vector<request_t>;
```

**Rationale:** Gives a named type to the parsed batch payload. Used by the
`message_type<request_t>` specialization and referenced at call sites in
`channel_rpc.ipp`. No `tag_invoke` serialization is needed because the reader
deserializes each array element individually using the existing
`value_to<request_t>`.

---

### 2. `include/bitcoin/network/messages/rpc/body.hpp`

**Changes:**

1. Full template specialization `message_type<request_t>` added after the
   generic template. The specialization inherits `json::json_value` (same base
   as the generic), adds a `batch_t batch{}` field alongside the existing
   `request_t message{}` field, and exposes `is_batch()`.

2. Doc comment added on `body<Message>::reader` describing the two-path
   contract (object root → single, array root → batch).

**Key invariants of the specialization:**
- `is_batch()` returns `!batch.empty()`.
- After a single-request parse: `batch` is empty, `message` is populated.
- After a batch parse: `batch` is populated, `message` is default-constructed.
- The two states are mutually exclusive by construction.

**Size impact:** `std::vector<request_t>` adds 24 bytes on 64-bit platforms.
`rpc::request` grows from ~248 to ~272 bytes. `rpc::response`
(`message_type<response_t>`, the generic instantiation) is unaffected.

---

### 3. `src/messages/rpc/body.cpp`

**Change:** `body<request_t>::reader::finish()` replaces the single
`value_to<request_t>` call with a three-branch dispatch:

| JSON root type | Action |
|----------------|--------|
| Object | Existing single-request path (deserialization + v1 semantic validation) |
| Array | New batch path: validate non-empty, iterate elements, reject non-objects, `value_to<request_t>` each one into `value_.batch` |
| Anything else | `jsonrpc_requires_method` (unchanged from old fallthrough) |

**Error policy in the batch path:** fail-fast. If any element is not a JSON
object or fails deserialization, `ec` is set immediately and the method returns.
The channel will call `stop(ec)`. No partial-batch recovery is attempted; the
client must send a structurally valid batch.

**Terminator guard added to `finish()`:** `terminated_ && !has_terminator_`
check added as an early-return guard. Previously this case was prevented by
`done()` returning false, but the explicit check makes the contract defensively
clear and matches the design described in the spec document.

---

### 4. `include/bitcoin/network/channels/channel_rpc.hpp`

**Changes:**

- **Two new private members:**
  - `rpc::request_cptr batch_source_{}` — holds the parsed request alive by
    shared_ptr during a batch. Zero-cost when no batch is in flight.
  - `size_t batch_cursor_{}` — cursor into `batch_source_->batch`.

- **New virtual method `dispatch_batch()`:** called from `handle_receive()` when
  `request->is_batch()`. The default implementation stores `batch_source_` and
  calls `dispatch_next()`. Derived channels that wish to reject batches may
  override and call `stop(error::not_implemented)`.

- **New non-virtual method `dispatch_next()`:** advances the cursor, skips
  notification items (no `id`), and dispatches the next request item. Non-virtual
  because the one-at-a-time sequencing is a correctness invariant, not a policy.

---

### 5. `include/bitcoin/network/impl/channels/channel_rpc.ipp`

**Changed methods:**

| Method | Change |
|--------|--------|
| `stopping()` | Resets `batch_source_` and `batch_cursor_` on stop |
| `handle_receive()` | Routes to `dispatch_batch()` or single path based on `is_batch()` |
| `handle_send()` | Final line changed: `receive()` replaced by conditional branch on `batch_source_` |

**New methods:**

| Method | Behaviour |
|--------|-----------|
| `dispatch_batch()` | Stores `batch_source_`/`batch_cursor_`, calls `dispatch_next()` |
| `dispatch_next()` | While-loop skips notifications; dispatches next request item; on batch exhaustion resets state and calls `receive()` |

**Resolved TODOs:** The existing TODO block in `handle_receive()` described
exactly this design (asynchronous iteration, no server-side buffering, per-item
delimiters). The comments are retained as `RESOLVED:` markers with a reference
to the implementation approach.

**Control-flow invariant:** At all times exactly one response write is in flight.
`handle_send()` drives `dispatch_next()` upon write completion. This gives
natural back-pressure: the client must consume each response before the server
reads the next batch item from its internal cursor.

---

### 6. `include/bitcoin/network/error.hpp`

**Change:** Two new enumerators appended after `jsonrpc_writer_exception`:

```cpp
jsonrpc_batch_empty,        // batch array must contain at least one element
jsonrpc_batch_item_invalid  // each batch element must be a JSON object
```

Both are in `error_t : uint8_t`. The enum has capacity; no overflow risk.

---

### 7. `src/error.cpp`

**Change:** Corresponding message strings added to `DEFINE_ERROR_T_MESSAGE_MAP`:

```cpp
{ jsonrpc_batch_empty,       "json-rpc batch array must not be empty" },
{ jsonrpc_batch_item_invalid, "json-rpc batch element is not a JSON object" },
```

---

## Test Coverage

### Implemented now

#### `test/messages/rpc/body_reader.cpp` (inside `HAVE_SLOW_TESTS`)

| Test case | What it covers |
|-----------|---------------|
| `message_type_specialization__default__is_not_batch_*` | Default state of specialization |
| `message_type_specialization__batch_with_one_element__is_batch` | `is_batch()` predicate in isolation |
| `finish__single_request_v2__success_*` | Regression: single-request path unchanged |
| `finish__batch_one_request__success_*` | Happy path: one-element TCP batch |
| `finish__batch_two_requests__success_*` | Happy path: order preservation, two items |
| `finish__batch_with_notification__*` | Notification (no `id`) is deserialized, `id` absent |
| `finish__batch_http_path_no_terminator__success` | HTTP path (non-terminated reader) |
| `finish__empty_array__jsonrpc_batch_empty_*` | Empty `[]` rejected |
| `finish__batch_element_is_number__jsonrpc_batch_item_invalid` | Non-object element (number) |
| `finish__batch_element_is_array__jsonrpc_batch_item_invalid` | Non-object element (array) |
| `finish__batch_valid_then_invalid_element__*` | Fail-fast on second invalid element |
| `finish__scalar_root__jsonrpc_requires_method` | Number root |
| `finish__string_root__jsonrpc_requires_method` | String root |

#### `test/error.cpp`

| Test case | What it covers |
|-----------|---------------|
| `error_t__code__jsonrpc_batch_empty__*` | Error code value, truthiness, message string |
| `error_t__code__jsonrpc_batch_item_invalid__*` | Error code value, truthiness, message string |

---

### Deferred / not yet implemented

#### `test/messages/rpc/body_reader.cpp` — stale test fixup

> **TODO:** The existing test cases in this file (above the `NOTE:` separator)
> use stale API names that do not compile against the current code:
>
> | Stale name | Correct name |
> |------------|-------------|
> | `rpc::body::value_type` | `rpc::request` |
> | `rpc::body::reader` | `rpc::reader` |
> | `body.request.*` | `body.message.*` |
> | `reader.is_done()` | `reader.done()` |
>
> These should be fixed in a follow-up commit before `HAVE_SLOW_TESTS` is
> enabled in CI.

#### `test/channels/channel_rpc.cpp` — channel dispatch tests

> **TODO:** The channel-level dispatch logic (`dispatch_batch`, `dispatch_next`,
> `handle_send` routing, `stopping` cleanup) requires a mock
> `channel_rpc`-compatible object that can:
>
> - Operate on a real or stub strand (for `BC_ASSERT(stranded())`)
> - Expose a controllable `dispatcher_` (to inject known-method handlers)
> - Record calls to `receive()` and `send_error()` / `send_result()`
>
> The current test suite has no such mock infrastructure. Once it exists, the
> following behaviors should be covered:
>
> | Scenario | Expected |
> |----------|---------|
> | `dispatch_next` — all notifications — cursor exhausted | `batch_source_` reset, `receive()` called |
> | `dispatch_next` — first item has `id` | subscriber fires with correct `identity_`/`version_` |
> | `dispatch_next` — item with unknown method | `send_error()` called; cursor advances via `handle_send` |
> | `handle_send` — `batch_source_` non-null | calls `dispatch_next()`, not `receive()` |
> | `handle_send` — `batch_source_` null | calls `receive()`, not `dispatch_next()` |
> | `stopping()` during batch | `batch_source_` reset, `batch_cursor_` = 0 |
> | `dispatch_batch()` when batch already in flight | `BC_ASSERT_MSG` fires ("nested batch") |
> | `dispatch_batch()` override rejecting batch | `stop(error::not_implemented)` |

#### HTTP batch dispatch (`protocol_bitcoind_rpc`)

> **TODO (Phase 3):** The network changes are necessary but not sufficient for
> HTTP batch. `channel_http` has no `handle_send → dispatch_next` loop.
> `protocol_bitcoind_rpc` must implement its own dispatch loop and choose a
> response strategy (Option A: JSON array response; Option B: chunked streaming).
> See `libbitcoin-server/docs/json-rpc-batch.md` §"HTTP Batch" for the trade-offs.

---

## Wire Behavior (Electrum TCP)

Client sends one newline-terminated message:

```
[{"jsonrpc":"2.0","id":1,"method":"blockchain.block.header","params":[100,0]},
 {"jsonrpc":"2.0","id":2,"method":"server.ping","params":[]},
 {"jsonrpc":"2.0","method":"server.ping","params":[]}]\n
```

Server sends two independent newline-terminated responses as they complete.
The third item (no `id`) is a notification and receives no response:

```
{"jsonrpc":"2.0","id":1,"result":{"header":"...","max":2016,"count":1}}\n
{"jsonrpc":"2.0","id":2,"result":null}\n
```

This deviates from JSON-RPC 2.0 §6 (which mandates a single array response)
but is the explicit design intent: push buffering to the client.

---

## Design Properties

| Property | This implementation |
|----------|-------------------|
| Server memory during batch | 1 response in flight; batch held via shared_ptr (no N-copy) |
| Back-pressure | Natural — each write completion drives the next dispatch |
| Parse failure policy | Fail-fast: any bad element stops the channel |
| Unknown method on request item | Error response sent; batch continues |
| Unknown method on notification | Silent skip |
| `http_body.hpp` changes | None |
| New public types | `batch_t` alias only |
| Existing API breakage | None |
