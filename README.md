# stegaSaur

### ---- Project Documents ----
Inception Writeup - https://docs.google.com/document/d/1X7kI5RTMygiNqbS4U0yBPI6-DcnI7eBbE7cWmsQKD0E/edit?usp=sharing

Design Document - https://docs.google.com/document/d/1pvw5EU2l6W-6iZrfrN1l9-DQl4fKrb1VNhXZi__MKUQ/edit?usp=sharing

---

### ---- Technical Memos ----
1. Basic LSB encoding for PNGs
   - Rationale: Easy to implement and simple logic
   - Alternatives:
     - Color-channel specific LSB: Enhances encoding by limiting LSB to a color-specific channel such as only encoding in the blue channel. This will hide the secret file better than basic lsb. We decided to continue with basic lsb for the sake of time and difficulty of implementation. We may consider implementing this method in the future
2. DCT encoding for JPEGs using the JSteg algorithm
    - Rationale: While libraries for JPEG steganography exist, we wanted to implement something ourselves.
    - Alternatives:
        - OutGuess: a steganography algorithm that also preserves the statistical properties of a JPEG to remain undetectable. Rejected because stealth is not one of our goals.
3. Basic LSB encoding for WAVs
    - Rationale: Code for PNG lsb was able to be reused for WAV encoding
    - Alternatives:
        - LSB Matching: checking to see if the byte we're overwritting already matches the byte to encode. We decided to reuse the basic LSB code to save time.
