# Nakuru Fresh Produce Market — Inventory, Order & Dispatch System

A small C++17 + HTML/CSS/vanilla JavaScript prototype for the Group 3 Systems Analysis & Design mini-SDLC assignment and a C++ OOP assignment.

## Features

- Single-page browser UI with seven views: dashboard, retailer order, batch intake, driver dispatch, delivery manifest, alerts and sales summary.
- C++17 domain model demonstrating encapsulation, abstraction, inheritance, polymorphism and composition.
- FIFO allocation: available batches for a produce item are sorted by earliest expiry date and stock is deducted from the oldest batch first.
- Input validation for positive quantities, valid retailer/driver/order references, sufficient stock and expiry date after arrival date.
- Seed data for Kenyan produce, retailers in Section 58/Milimani/Lanet and drivers with Kenyan vehicle registrations.
- CSV snapshot persistence in `data/market.csv` after mutations.

## Requirements

- Ubuntu/Linux with `g++` supporting C++17.
- No third-party C++ libraries or database server are required. The tiny HTTP server uses Linux/POSIX sockets.

## Compile and run

From the project directory:

```bash
g++ -std=c++17 -O2 main.cpp -o market_app
./market_app
```

Open **http://localhost:8080** in your browser. Stop the server with `Ctrl+C`.

The server expects `index.html` in the current working directory. It creates `data/` automatically and writes `data/market.csv` when an order, batch or dispatch is saved.

## API routes

| Method | Route | Purpose |
|---|---|---|
| GET | `/api/dashboard` | KPI values |
| GET | `/api/inventory` | Inventory sorted by expiry date |
| GET | `/api/products` | Produce list and live available stock |
| GET | `/api/retailers` | Retailer dropdown data |
| GET | `/api/orders` | Recorded orders |
| GET | `/api/drivers` | Driver dropdown data |
| GET | `/api/manifest` | Driver delivery manifest |
| GET | `/api/alerts` | Expiry and low-stock exceptions |
| GET | `/api/sales` | Revenue totals by produce |
| POST | `/api/orders` | Create an order and allocate FIFO stock |
| POST | `/api/batches` | Record a new produce batch |
| POST | `/api/dispatch` | Assign an order to a driver |

Example order payload:

```json
{"retailerId":"R001","channel":"WhatsApp","produce":"Tomatoes","quantity":15}
```

## OOP concept mapping for the C++ report

| OOP concept | Where it appears | Explanation |
|---|---|---|
| Encapsulation | `ProduceBatch`, `Order`, `Retailer`, `Delivery` | Domain data is private. Constructors and setters validate quantities and batch dates before changing state. |
| Abstraction | `MarketEntity` | Abstract base class defines the common `toJson()` and `getSummary()` interface. |
| Inheritance | `Retailer : public Person`, `Driver : public Person` | Retailer and Driver reuse the common person identity, name and phone attributes. |
| Polymorphism | `MarketEntity` subclasses and `ReportGenerator` | `ReportGenerator::render(const vector<MarketEntity*>&)` calls overridden `toJson()` methods through base-class pointers. |
| Composition | `InventoryManager` | InventoryManager owns the collections of batches, retailers, drivers, orders and deliveries and coordinates their business rules. |

## FIFO business rule

When an order is submitted, the system:

1. Calculates total available stock for the selected produce.
2. Rejects the order if the requested quantity exceeds available stock.
3. Sorts eligible batches by `expiryDate` ascending.
4. Deducts the requested quantity from the earliest-expiring batch, continuing into the next batch only when necessary.
5. Stores the first allocated batch ID on the order for the demo manifest.

Low stock is reported when the remaining total for a produce category falls below its reorder level. Batches expiring within the next two calendar days are shown in the exception report; date-only prototype data is used rather than hour-level timestamps.

## Prototype boundaries

This is an educational proof of concept, not a production deployment. The HTTP server is intentionally small and single-process. The flat-file CSV snapshot is intended for a local demonstration; it is not a concurrent multi-user database. Authentication, secure password storage, CSRF protection, HTTPS termination, production-grade JSON parsing, database migrations and cloud backups are not implemented. For a production system, use a maintained HTTP framework, a real database, secure authentication and a reverse proxy.

## Assignment alignment

The prototype supports the design documented in the Group 3 analysis: retailer order capture, batch intake with arrival/expiry dates, FIFO stock allocation, dispatch scheduling, a printable manifest, low-stock/spoilage alerts and sales reporting. The project documentation and diagrams remain separate assignment deliverables; this repository supplies the programming prototype and OOP implementation.
