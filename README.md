
# es9018k2m kernel module

Based on information freely available from the internet and prior code.

Modernized for kernel 6.1.69-v8+

## Fixed Issues

- **CRITICAL**: Removed kernel panic caused by `mdelay(1500)` in atomic context - replaced with proper delayed workqueue
- Replaced `mdelay()` with `msleep()` in probe function
- Added `of_node_put()` to fix device tree node reference leak
- Updated to modern I2C probe signature (single argument)
- Converted to `devm_` managed resource functions for automatic cleanup
- Removed deprecated `.owner` field from platform driver
- Removed unused global variables

The driver should now be safe to use without kernel panics.
