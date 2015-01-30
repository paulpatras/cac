# CAC
# Author: Paul Patras

CAC is a Linux application that implements a centralized adaptive algorithm that can be 
deployed on commodity access points to optimise the throughput performance of WLANs based
on the IEEE 802.11 technology.

CAC targets the best-effort (BE) access category and was developed in conjunction with a 
modified version of the popular Madwifi driver, whereby the update of the contention parameters 
assigned to the BE access category was enabled. The modified driver is available at:

https://github.com/paulpatras/madwifi-be

Details about CAC's operation are documented in the flowing research papers:

- P. Patras, A. Banchs, P. Serrano, "A Control Theoretic Approach for Throughput Optimization 
in IEEE 802.11e EDCA WLANs", ACM/Springer Mobile Networks and Applications (MONET), vol. 14, 
no. 6, pp. 697–708, Dec. 2009.
- P. Serrano, P. Patras, A. Mannocci, V. Mancuso, A. Banchs, "Control Theoretic Optimization 
of 802.11 WLANs: Implementation and Experimental Evaluation", Computer Networks, vol. 57, 
no. 1, pp. 258–272, Jan. 2013.
