# OpenShard

### Provider-Side Selective Access for Large Data Archives

> **Download the data you need, not the archive that contains it.**

OpenShard is an experimental C++ project exploring a provider-side access layer for large public datasets stored as archives or shards.

Large datasets are often optimized for bulk distribution. A provider may store millions of files inside large TAR archives or other shards:

```text
dataset
├── shard-001.tar
├── shard-002.tar
├── shard-003.tar
└── ...
```

This is efficient for distributing the complete dataset.

However, a client may only need a small subset:

```text
Dataset:       100 GB
Files:         millions
Client needs:  20 files
Required data: 80 MB
```

A conventional workflow can force the client to retrieve a large amount of data just to obtain a small subset.

OpenShard investigates a different approach:

> **Keep the archive and its indexing infrastructure at the provider side, and expose logical file-level access to clients.**

The client does not need to download the archive, inspect its structure, build an index, or know where the requested file is physically stored.

---

# Core Idea

Instead of:

```text
                    CLIENT
                       │
                       │
                 Download archive
                       │
                       ↓
                  100 GB TAR
                       │
                       ↓
                  Find 20 files
                       │
                       ↓
                    80 MB
```

OpenShard aims for:

```text
                    CLIENT
                       │
                 "Give me file X"
                       │
                       ↓
                ┌──────────────┐
                │   OpenShard  │
                │    Gateway   │
                └──────┬───────┘
                       │
                Provider-side
                     index
                       │
                 byte location
                       │
                       ↓
                Object Storage
                       │
                 Range Retrieval
                       │
                       ↓
                    File X
                       │
                       ↓
                    CLIENT
```

The underlying archive remains the provider's canonical dataset.

---

# Why OpenShard?

The interesting problem is not simply:

> "How do I download part of a TAR?"

The deeper question is:

> **How can a provider expose efficient sparse access to a large immutable archive without forcing every client to understand or download the archive itself?**

This creates a separation between:

### Physical storage

```text
TAR
shards
object storage
byte offsets
```

and:

### Logical access

```text
dataset
    ↓
document
    ↓
file
```

The client interacts with the logical representation.

The provider handles the physical representation.

---

# Architecture

```text
                         PROVIDER
                            │
          ┌─────────────────┴─────────────────┐
          │                                   │
    Existing Archives                    Provider Index
          │                                   │
      ┌───┴────┐                     file → archive
      │        │                     archive → offset
    TAR 1    TAR 2                   offset → size
      │        │                           │
      └───┬────┘                           │
          │                                 │
          └──────────────┬──────────────────┘
                         ↓
                 ┌───────────────┐
                 │   OpenShard    │
                 │    Gateway     │
                 └───────┬───────┘
                         │
                      HTTP API
                         │
                         ↓
                       CLIENT
```

Internally:

```text
Request
   ↓
File resolution
   ↓
Index lookup
   ↓
Archive location
   ↓
Offset + size
   ↓
Range retrieval
   ↓
Archive extraction
   ↓
HTTP response
```

---

# Design Principles

## 1. Provider-side intelligence

The client should not need to:

* download large archives
* scan TAR files
* build indexes
* understand archive layouts
* calculate byte offsets
* know the underlying object-storage structure

---

## 2. Preserve existing datasets

OpenShard is not intended to require providers to migrate their existing datasets into a new proprietary format.

The goal is to place an access layer over existing storage.

```text
Existing Dataset
       +
Provider-side Index
       +
OpenShard Gateway
       =
Selective Access
```

---

## 3. HTTP as the compatibility boundary

The gateway is implemented in C++, but clients do not need to use C++.

Any HTTP-capable client can interact with it:

```text
Python
Java
JavaScript
Go
Rust
C++
curl
```

For example:

```http
GET /datasets/dataset-a/files/document-123.pdf
```

The client receives the requested file without needing to know how it is stored.

---

## 4. Measure before optimizing

OpenShard will not assume that selective retrieval is always better.

For some workloads:

```text
Request 1 file
→ selective access
```

may be appropriate.

For another workload:

```text
Request 90% of dataset
→ bulk download
```

may be more efficient.

The project will measure the tradeoffs.

---

# Core Metrics

## Transfer Amplification

The primary metric is:

```text
Transfer Amplification =
Bytes Transferred / Bytes Requested
```

Example:

```text
Archive:          100 GB
Requested data:   50 MB
Transferred:      100 GB

Amplification = 2000×
```

A selective system could potentially reduce this substantially.

Actual results will be measured rather than assumed.

---

## Other Metrics

OpenShard will evaluate:

* request latency
* useful throughput
* bytes transferred
* number of object-storage requests
* client storage requirements
* provider CPU usage
* provider memory usage
* index generation time
* index size
* concurrent-request performance
* cache effectiveness

---

# Phase-by-Phase Development

The project will be developed incrementally.

Each phase should produce a working result before the next phase begins.

---

## Phase 0 — Validate Remote Range Retrieval

### Goal

Prove that the C++ system can retrieve only a requested byte range from remote object storage.

```text
C++ program
     ↓
Remote object
     ↓
Range request
     ↓
Required bytes
```

### Build

A minimal C++ storage client that can:

* identify an object
* request a byte range
* receive the response
* measure transferred bytes
* measure latency

### Output

Example benchmark:

```text
Object size:          20 GB
Requested range:     4.2 MB
Bytes received:      4.2 MB
Latency:             180 ms
```

### Why this phase matters

Before building OpenShard, we need to prove that the fundamental retrieval mechanism works.

---

# Phase 1 — Archive Reader

### Goal

Understand and extract individual files from an archive.

```text
TAR
 │
 ├── file A
 ├── file B
 ├── file C
 └── ...
```

### Build

A C++ archive reader capable of:

* reading TAR headers
* identifying files
* determining file offsets
* determining file sizes
* extracting selected files

### Output

Conceptually:

```text
archive.tar
      ↓
find document-123.pdf
      ↓
offset = X
size   = Y
```

---

# Phase 2 — Provider-Side Index

### Goal

Avoid scanning an archive every time a client requests a file.

Generate an index such as:

```json
{
  "document-123.pdf": {
    "archive": "shard-017.tar",
    "offset": 8291102144,
    "size": 4238192
  }
}
```

### Build

The indexing system will record:

```text
file identifier
archive
offset
size
```

Additional metadata can later include:

```text
checksum
dataset
version
content type
compression information
```

### Important

The index is created and maintained **at the provider side**.

The client never downloads it just to locate a file.

---

# Phase 3 — Selective Retrieval Engine

### Goal

Combine:

```text
Index
+
Range retrieval
+
Archive extraction
```

into one provider-side retrieval engine.

Request:

```text
document-123.pdf
```

Internal process:

```text
document-123.pdf
       ↓
Index lookup
       ↓
shard-017.tar
       ↓
offset + size
       ↓
Range request
       ↓
Extract file
       ↓
Return file
```

At this point OpenShard should already be able to demonstrate the core idea without an HTTP server.

---

# Phase 4 — C++ HTTP Gateway

### Goal

Expose the retrieval engine through a language-independent interface.

Example:

```http
GET /datasets/legal/files/document-123.pdf
```

The gateway internally performs:

```text
HTTP request
     ↓
dataset resolution
     ↓
file lookup
     ↓
range retrieval
     ↓
extraction
     ↓
HTTP response
```

### Client experience

The client only needs:

```bash
curl \
  https://provider.example/datasets/legal/files/document-123.pdf \
  -o document-123.pdf
```

No OpenShard client library is required.

---

# Phase 5 — Multi-File Retrieval

### Goal

Support requests for multiple files.

Example:

```http
POST /datasets/legal/files
```

```json
{
  "files": [
    "document-1.pdf",
    "document-2.pdf",
    "document-3.pdf"
  ]
}
```

The gateway resolves all requested files.

This introduces a new problem:

> How should multiple sparse requests be retrieved efficiently?

---

# Phase 6 — Retrieval Planner

This is where the project becomes more interesting than a simple file-access API.

Suppose:

```text
A → shard-1 → 100 MB
B → shard-1 → 104 MB
C → shard-1 → 108 MB
D → shard-9 → 8 GB
```

Naively:

```text
Range A
Range B
Range C
Range D
```

The planner can investigate alternatives:

```text
Retrieve one larger nearby range
        ↓
Extract A, B, C
```

versus:

```text
Retrieve three separate ranges
```

The planner can consider:

```text
number of requests
transfer size
range distance
latency
server resources
cache state
```

The goal is not to assume one strategy is best.

The goal is to **measure and compare retrieval strategies**.

---

# Phase 7 — Caching

### Goal

Reduce repeated access to popular data.

Possible cache levels:

```text
File cache
    ↓
Range cache
    ↓
Archive cache
```

The project will investigate whether caching:

* reduces object-storage requests
* reduces latency
* changes the optimal retrieval strategy

Caching remains optional to the core architecture.

---

# Phase 8 — Concurrency and Scalability

### Goal

Evaluate the gateway under concurrent workloads.

Example:

```text
             ┌── Request 1
             ├── Request 2
Client ──────┼── Request 3
             ├── Request 4
             └── Request 5
                     ↓
                OpenShard
                     ↓
                Object Store
```

Measure:

* requests/second
* latency distribution
* memory usage
* CPU usage
* concurrent range requests
* storage throughput

---

# Phase 9 — Evaluation

The final system will be compared against conventional bulk access.

### Baseline

```text
Client
  ↓
Download archive
  ↓
Search/extract files
```

### OpenShard

```text
Client
  ↓
Request files
  ↓
Provider-side index
  ↓
Selective range retrieval
  ↓
Files
```

The comparison will use different access patterns.

### Sparse

```text
1–10 files
```

### Very sparse

```text
<0.01% of dataset
```

### Moderate

```text
1–20%
```

### Dense

```text
20%+
```

This will help identify **when selective access is beneficial and when bulk retrieval remains preferable**.

---

# Phase 10 — Generalization

The first implementation will focus on TAR-based datasets.

After the core architecture has been validated, possible future formats include:

```text
TAR
ZIP
sharded datasets
partitioned archives
other immutable archive layouts
```

The project will only expand to additional formats if the experiments justify doing so.

---

# Initial Case Study

The first motivating case study is a public Indian High Court judgments dataset.

The current access pattern can involve:

```text
Large TAR archive
       ↓
Locate relevant archive
       ↓
Download archive
       ↓
Scan archive
       ↓
Extract required PDFs
```

OpenShard will investigate:

```text
Client
   ↓
Request judgment
   ↓
Provider-side lookup
   ↓
Selective retrieval
   ↓
Judgment PDF
```

The Indian High Court dataset is a **case study**, not the limitation of the system.

---

# Project Structure

The initial C++ project is expected to evolve toward:

```text
openshard/
│
├── CMakeLists.txt
├── README.md
│
├── include/
│   ├── archive/
│   ├── index/
│   ├── storage/
│   ├── retrieval/
│   └── gateway/
│
├── src/
│   ├── archive/
│   ├── index/
│   ├── storage/
│   ├── retrieval/
│   └── gateway/
│
├── tests/
│
├── benchmarks/
│
├── examples/
│
└── docs/
```

The exact structure may change as the architecture is validated.

---

# Technology

### Core

* C++
* CMake

### Provider Interface

* HTTP
* REST-style API

### Storage

Initially:

* S3-compatible object storage
* HTTP Range Requests

Potentially later:

* MinIO
* local object storage
* other object-storage systems

### Archive Format

Initial:

* TAR

Potential future formats will depend on evaluation results.

---

# What OpenShard Is Not

OpenShard is **not**:

* a generic downloader
* a TAR extraction utility
* a client-side indexing library
* a replacement for S3
* a new dataset storage format
* an AI-powered downloader
* a requirement for clients to install C++ software
* a system that assumes selective retrieval is always faster

The goal is specifically to investigate **provider-side selective access to large existing archives**.

---

# Research Hypothesis

The initial hypothesis is:

> **For sufficiently sparse access patterns, provider-side indexing combined with selective range retrieval can substantially reduce client-side data transfer and storage compared with downloading the complete archive.**

This is a hypothesis to be tested.

The project will not treat it as a proven result until benchmarks across multiple datasets and workloads support it.

---

# Current Status

🚧 **Experimental — Phase 0**

Current priorities:

1. Validate remote byte-range retrieval in C++.
2. Measure actual transfer behavior.
3. Study the archive structure of real public datasets.
4. Establish transfer amplification.
5. Determine whether the proposed provider-side architecture provides a meaningful advantage.
6. Build the remaining phases only after the evidence supports the design.

---

# Long-Term Goal

OpenShard aims to make large public datasets behave more like logical file systems from the client's perspective:

```text
Client:

"I need these 5 files."

Provider:

"Here they are."

```

without requiring the client to know that those files are physically buried inside a massive archive.

---

## License

License to be determined.
