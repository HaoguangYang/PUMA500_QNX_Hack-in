Zynq DMA LIBRARY USER'S GUIDE

The Zynq can support up to 8 DMA channels.

API OPTIONS:

init() options = irq=i,regbase=0xaaaaaaaa
 defaults: irq=45, regbase = 0xF8004000

channel_attach()
    
  The 'regen' option tells the library to automatically re-enable the descriptor ring.
  The 'contloop' tells the dma engine to continously repeat a transfer

    
Supported Attach Flags:
	DMA_ATTACH_EVENT_ON_COMPLETE =	0x00000010,	/* Want an event on transfer completion */
	DMA_ATTACH_EVENT_PER_SEGMENT =	0x00000020,	/* Want an event per fragment transfer completion */

Supported Xfer Flags:
NONE
