#!/usr/bin/env python3
#
#    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

"""Generates the XHAL Crypto test suite vectors.

Inputs come from published standards (NIST SP 800-38A/B/C, FIPS 180-4,
the GCM specification and NIST CAVP, RFC 4231, RFC 5869); expected outputs
are computed with the Python "cryptography" package, hashlib and hmac, then
checked against values printed in those documents before anything is
written. Long vectors use a deterministic pattern and exercise backend paths,
such as DMA, that short standard vectors do not reach.

Usage, from this directory:

    python3 xcry_gen_vectors.py

Writes ../source/testref/xcry_vectors.c and ../source/testref/xcry_vectors.h.
"""

import hashlib
import hmac
import os

from cryptography.hazmat.primitives import cmac, hashes
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.ciphers.aead import AESCCM, AESGCM
from cryptography.hazmat.primitives.kdf.hkdf import HKDF

OUTDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      '..', 'source', 'testref')

LONG_SIZE = 4096
LONG_PATTERN = bytes(((i * 167) + 13) & 0xFF for i in range(LONG_SIZE))


def h(s):
    return bytes.fromhex(s.replace(' ', ''))


def check(what, value, published):
    if value != h(published):
        raise SystemExit('generator self-check failed: %s' % what)


# NIST SP 800-38A, appendix F.
SP38A_KEYS = {
    128: h('2b7e151628aed2a6abf7158809cf4f3c'),
    192: h('8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b'),
    256: h('603deb1015ca71be2b73aef0857d7781'
           '1f352c073b6108d72d9810a30914dff4'),
}
SP38A_PT = h('6bc1bee22e409f96e93d7e117393172a'
             'ae2d8a571e03ac9c9eb76fac45af8e51'
             '30c81c46a35ce411e5fbc1191a0a52ef'
             'f69f2445df4f9b17ad2b417be66c3710')
SP38A_IV = h('000102030405060708090a0b0c0d0e0f')
SP38A_CTR = h('f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff')

CIPHER_MODES = {
    'ECB':    (lambda iv: modes.ECB(), None, 'F.1.%d', 'ECB-AES%d'),
    'CBC':    (lambda iv: modes.CBC(iv), SP38A_IV, 'F.2.%d', 'CBC-AES%d'),
    'CFB128': (lambda iv: modes.CFB(iv), SP38A_IV, 'F.3.%d', 'CFB128-AES%d'),
    'CTR':    (lambda iv: modes.CTR(iv), SP38A_CTR, 'F.5.%d', 'CTR-AES%d'),
}
SP38A_SECTION_BASE = {'ECB': 1, 'CBC': 1, 'CFB128': 13, 'CTR': 1}


def aes_encrypt(mode, key, iv, data):
    enc = Cipher(algorithms.AES(key), CIPHER_MODES[mode][0](iv)).encryptor()
    return enc.update(data) + enc.finalize()


# GCM specification (McGrew, Viega) test cases and NIST CAVP. Entries are
# (title, key, nonce, AAD, plaintext, published 128-bit tag, tag size); a
# shorter tag is the leading part of the full tag. The first entry is used
# by the authentication failure test. Nonces other than 96 bits are
# optional for backends.
GCM_TC_P = h('d9313225f88406e5a55909c5aff5269a'
             '86a7a9531534f7da2e4c303d8a318a72'
             '1c3c0c95956809532fcf0e2449a6b525'
             'b16aedf5aa0de657ba637b39')
GCM_TC3_P = GCM_TC_P + h('1aafd255')
GCM_TC_A = h('feedfacedeadbeeffeedfacedeadbeefabaddad2')
GCM_TC_N = h('cafebabefacedbaddecaf888')
GCM_TC_K128 = h('feffe9928665731c6d6a8f9467308308')
GCM_TC_K192 = h('feffe9928665731c6d6a8f9467308308feffe9928665731c')
GCM_TC_K256 = h('feffe9928665731c6d6a8f9467308308'
                'feffe9928665731c6d6a8f9467308308')
GCM_VECTORS = [
    ('GCM spec test case 4, AES-128', GCM_TC_K128,
     GCM_TC_N, GCM_TC_A, GCM_TC_P, '5bc94fbc3221a5db94fae95ae7121a47', 16),
    ('GCM spec test case 16, AES-256', GCM_TC_K256,
     GCM_TC_N, GCM_TC_A, GCM_TC_P, '76fc6ece0f4e1768cddf8853bb2d551b', 16),
    ('CAVP gcmEncryptExtIV128, 128-bit P',
     h('c939cc13397c1d37de6ae0e1cb7c423c'), h('b3d8cc017cbb89b39e0f67e2'),
     h('24825602bd12a984e0092d3e448eda5f'),
     h('c3b3c41f113a31b73d9a5cd432103069'),
     '0032a1dc85f1c9786925a2e71d8272dd', 16),
    ('CAVP gcmEncryptExtIV128, 256-bit P',
     h('298efa1ccf29cf62ae6824bfc19557fc'), h('6f58a93fe1d207fae4ed2f6d'),
     h('021fafd238463973ffe80256e5b1c6b1'),
     h('cc38bccd6bc536ad919b1395f5d63801f99f8068d65ca5ac63872daf16b93901'),
     '542465ef599316f73a7a560509a2d9f2', 16),
    ('GCM spec test case 1, empty AAD and P', bytes(16), bytes(12), b'', b'',
     '58e2fccefa7e3061367f1d57a4e7455a', 16),
    ('GCM spec test case 2, empty AAD', bytes(16), bytes(12), b'', bytes(16),
     'ab6e47d42cec13bdf53a67b21257bddf', 16),
    ('GCM spec test case 3, empty AAD', GCM_TC_K128, GCM_TC_N, b'', GCM_TC3_P,
     '4d5c2af327cd64a62cf35abd2ba6fab4', 16),
    ('GCM spec test case 7, AES-192, empty AAD and P', bytes(24), bytes(12),
     b'', b'', 'cd33b28ac773f74ba00ed1f312572435', 16),
    ('GCM spec test case 10, AES-192', GCM_TC_K192,
     GCM_TC_N, GCM_TC_A, GCM_TC_P, '2519498e80f1478f37ba55bd6d27618c', 16),
    ('CAVP gcmEncryptExtIV128, empty P',
     h('77be63708971c4e240d1cb79e8d77feb'), h('e0e00f19fed7ba0136a797f3'),
     h('7a43ec1d9c0a5a78a0b16533a6213cab'), b'',
     '209fcc8d3675ed938e9c7166709dd946', 16),
    ('GCM spec test case 4, 96-bit tag', GCM_TC_K128,
     GCM_TC_N, GCM_TC_A, GCM_TC_P, '5bc94fbc3221a5db94fae95ae7121a47', 12),
    ('GCM spec test case 4, 64-bit tag', GCM_TC_K128,
     GCM_TC_N, GCM_TC_A, GCM_TC_P, '5bc94fbc3221a5db94fae95ae7121a47', 8),
    ('GCM spec test case 4, 32-bit tag', GCM_TC_K128,
     GCM_TC_N, GCM_TC_A, GCM_TC_P, '5bc94fbc3221a5db94fae95ae7121a47', 4),
    ('GCM spec test case 5, 64-bit nonce', GCM_TC_K128,
     h('cafebabefacedbad'), GCM_TC_A, GCM_TC_P,
     '3612d2e79e3b0785561be14aaca2fccb', 16),
    ('GCM spec test case 6, 480-bit nonce', GCM_TC_K128,
     h('9313225df88406e555909c5aff5269aa'
       '6a7a9538534f7da1e4c303d2a318a728'
       'c3c0c95156809539fcf0e2429a6b5254'
       '16aedbf5a0de6a57a637b39b'), GCM_TC_A, GCM_TC_P,
     '619cc5aefffe0bfa462af43c1699d050', 16),
]

# NIST SP 800-38C, appendix C.
CCM_KEY = h('404142434445464748494a4b4c4d4e4f')
CCM_VECTORS = [
    ('SP800-38C C.1', h('10111213141516'), h('0001020304050607'),
     h('20212223'), 4, '7162015b4dac255d'),
    ('SP800-38C C.2', h('1011121314151617'),
     h('000102030405060708090a0b0c0d0e0f'),
     h('202122232425262728292a2b2c2d2e2f'), 6,
     'd2a1f0e051ea5f62081a7792073d593d1fc64fbfaccd'),
    ('SP800-38C C.3', h('101112131415161718191a1b'),
     h('000102030405060708090a0b0c0d0e0f10111213'),
     h('202122232425262728292a2b2c2d2e2f3031323334353637'), 8,
     'e3b201a9f5b71a7a9b1ceaeccd97e70b6176aad9a4428aa5484392fbc1b09951'),
]

# NIST SP 800-38B, appendix D, message lengths 0, 128, 320 and 512 bits.
CMAC_LENGTHS = (0, 16, 40, 64)

# RFC 4231 test cases 1, 2, 3, 4, 6 and 7.
HMAC_CASES = [
    ('RFC 4231 TC1', b'\x0b' * 20, b'Hi There'),
    ('RFC 4231 TC2', b'Jefe', b'what do ya want for nothing?'),
    ('RFC 4231 TC3', b'\xaa' * 20, b'\xdd' * 50),
    ('RFC 4231 TC4', bytes(range(1, 26)), b'\xcd' * 50),
    ('RFC 4231 TC6', b'\xaa' * 131,
     b'Test Using Larger Than Block-Size Key - Hash Key First'),
    ('RFC 4231 TC7', b'\xaa' * 131,
     b'This is a test using a larger than block-size key and a larger '
     b'than block-size data. The key needs to be hashed before being '
     b'used by the HMAC algorithm.'),
]
HMAC_ALGS = [('HMAC_SHA256', hashlib.sha256), ('HMAC_SHA384', hashlib.sha384),
             ('HMAC_SHA512', hashlib.sha512)]

# FIPS 180-4 examples.
SHA_MESSAGES = [
    ('empty message', b''),
    ('"abc"', b'abc'),
    ('448-bit message',
     b'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq'),
    ('896-bit message',
     b'abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn'
     b'hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu'),
]
SHA_ALGS = [('SHA1', hashlib.sha1), ('SHA224', hashlib.sha224),
            ('SHA256', hashlib.sha256), ('SHA384', hashlib.sha384),
            ('SHA512', hashlib.sha512)]
MILLION_A = b'a' * 1000000

# RFC 5869 test cases 1, 2 and 3, SHA-256.
HKDF_CASES = [
    ('RFC 5869 TC1', b'\x0b' * 22, bytes(range(0x00, 0x0d)),
     bytes(range(0xf0, 0xfa)), 42),
    ('RFC 5869 TC2', bytes(range(0x00, 0x50)), bytes(range(0x60, 0xb0)),
     bytes(range(0xb0, 0x100)), 82),
    ('RFC 5869 TC3', b'\x0b' * 22, b'', b'', 42),
]


class Emitter:

    def __init__(self):
        self.arrays = []
        self.tables = []
        self.decls = []
        self.names = {}
        self.count = 0

    def array(self, data, prefix='xcry_data'):
        """Emits a byte array once per content, returns its name, or NULL
        when empty."""
        if len(data) == 0:
            return 'NULL'
        if data in self.names:
            return self.names[data]
        self.count += 1
        name = '%s_%d' % (prefix, self.count)
        lines = []
        for i in range(0, len(data), 12):
            lines.append('  ' + ', '.join('0x%02x' % b for b in data[i:i + 12]))
        self.arrays.append('static const uint8_t %s[%d] = {\n%s\n};\n'
                           % (name, len(data), ',\n'.join(lines)))
        self.names[data] = name
        return name

    def table(self, ctype, name, rows, comment):
        body = ',\n'.join('  {\n' + ',\n'.join('    ' + f for f in r) + '\n  }'
                          for r in rows)
        self.tables.append('/* %s */\nconst %s %s[%d] = {\n%s\n};\n'
                           % (comment, ctype, name, len(rows), body))
        self.decls.append('extern const %s %s[%d];' % (ctype, name, len(rows)))
        self.decls.append('#define %s_COUNT %dU' % (name.upper(), len(rows)))


def cstr(s):
    return '"%s"' % s.replace('"', '\\"')


def buf(e, data):
    return ['%dU' % len(data), e.array(data)]


def main():
    e = Emitter()

    # Cipher vectors.
    for mode in ('ECB', 'CBC', 'CFB128', 'CTR'):
        iv = CIPHER_MODES[mode][1]
        rows = []
        for n, bits in enumerate((128, 192, 256)):
            key = SP38A_KEYS[bits]
            ct = aes_encrypt(mode, key, iv, SP38A_PT)
            section = CIPHER_MODES[mode][2] % (SP38A_SECTION_BASE[mode] + 2 * n)
            title = 'SP800-38A %s %s' % (section, CIPHER_MODES[mode][3] % bits)
            rows.append([cstr(title), 'CRY_ALG_AES_%s' % mode] +
                        buf(e, key) + buf(e, iv or b'') + buf(e, SP38A_PT) +
                        [e.array(ct)])
        e.table('xcry_cipher_vector_t', 'xcry_aes_%s_vectors' % mode.lower(),
                rows, 'AES-%s, NIST SP 800-38A.' % mode)
        key = SP38A_KEYS[128]
        ct = aes_encrypt(mode, key, iv, LONG_PATTERN)
        e.table('xcry_cipher_vector_t', 'xcry_aes_%s_long' % mode.lower(),
                [[cstr('AES-128 %s, %d bytes pattern' % (mode, LONG_SIZE)),
                  'CRY_ALG_AES_%s' % mode] + buf(e, key) + buf(e, iv or b'') +
                 buf(e, LONG_PATTERN) + [e.array(ct)]],
                'AES-%s long message, generated.' % mode)
    check('SP800-38A F.1.1', aes_encrypt('ECB', SP38A_KEYS[128], None,
                                         SP38A_PT)[:16],
          '3ad77bb40d7a3660a89ecaf32466ef97')
    check('SP800-38A F.2.1', aes_encrypt('CBC', SP38A_KEYS[128], SP38A_IV,
                                         SP38A_PT)[:16],
          '7649abac8119b246cee98e9b12e9197d')
    check('SP800-38A F.3.13', aes_encrypt('CFB128', SP38A_KEYS[128], SP38A_IV,
                                          SP38A_PT)[:16],
          '3b3fd92eb72dad20333449f8e83cfb4a')
    check('SP800-38A F.5.1', aes_encrypt('CTR', SP38A_KEYS[128], SP38A_CTR,
                                         SP38A_PT)[:16],
          '874d6191b620e3261bef6864990db6ce')

    # AEAD vectors.
    rows = []
    for title, key, nonce, aad, pt, tag, tlen in GCM_VECTORS:
        out = AESGCM(key).encrypt(nonce, pt, aad)
        check(title, out[-16:], tag)
        rows.append([cstr(title), 'CRY_ALG_AES_GCM'] + buf(e, key) +
                    buf(e, nonce) + buf(e, aad) + buf(e, pt) +
                    [e.array(out[:-16]), '%dU' % tlen,
                     e.array(out[-16:][:tlen])])
    e.table('xcry_aead_vector_t', 'xcry_aes_gcm_vectors', rows,
            'AES-GCM, GCM specification and NIST CAVP.')
    key, nonce, aad = SP38A_KEYS[128], GCM_TC_N, GCM_TC_A
    out = AESGCM(key).encrypt(nonce, LONG_PATTERN, aad)
    e.table('xcry_aead_vector_t', 'xcry_aes_gcm_long',
            [[cstr('AES-128 GCM, %d bytes pattern' % LONG_SIZE),
              'CRY_ALG_AES_GCM'] + buf(e, key) + buf(e, nonce) + buf(e, aad) +
             buf(e, LONG_PATTERN) + [e.array(out[:-16]), '16U',
                                     e.array(out[-16:])]],
            'AES-GCM long message, generated.')
    rows = []
    for title, nonce, aad, pt, tlen, published in CCM_VECTORS:
        out = AESCCM(CCM_KEY, tag_length=tlen).encrypt(nonce, pt, aad)
        check(title, out, published)
        rows.append([cstr(title), 'CRY_ALG_AES_CCM'] + buf(e, CCM_KEY) +
                    buf(e, nonce) + buf(e, aad) + buf(e, pt) +
                    [e.array(out[:-tlen]), '%dU' % tlen,
                     e.array(out[-tlen:])])
    e.table('xcry_aead_vector_t', 'xcry_aes_ccm_vectors', rows,
            'AES-CCM, NIST SP 800-38C.')

    # MAC vectors.
    rows = []
    for bits in (128, 192, 256):
        for n in CMAC_LENGTHS:
            c = cmac.CMAC(algorithms.AES(SP38A_KEYS[bits]))
            c.update(SP38A_PT[:n])
            tag = c.finalize()
            rows.append([cstr('SP800-38B D, AES-%d, Mlen %d' % (bits, 8 * n)),
                         'CRY_ALG_AES_CMAC'] + buf(e, SP38A_KEYS[bits]) +
                        buf(e, SP38A_PT[:n]) + buf(e, tag))
            if bits == 128 and n == 0:
                check('SP800-38B D.1 example 1', tag,
                      'bb1d6929e95937287fa37d129b756746')
    e.table('xcry_mac_vector_t', 'xcry_aes_cmac_vectors', rows,
            'AES-CMAC, NIST SP 800-38B.')
    rows = []
    for alg, fn in HMAC_ALGS:
        for title, key, msg in HMAC_CASES:
            tag = hmac.new(key, msg, fn).digest()
            if alg == 'HMAC_SHA256' and title == 'RFC 4231 TC1':
                check(title, tag, 'b0344c61d8db38535ca8afceaf0bf12b'
                                  '881dc200c9833da726e9376c2e32cff7')
            rows.append([cstr('%s %s' % (title, alg.replace('_', '-'))),
                         'CRY_ALG_%s' % alg] + buf(e, key) + buf(e, msg) +
                        buf(e, tag))
    e.table('xcry_mac_vector_t', 'xcry_hmac_vectors', rows,
            'HMAC-SHA2, RFC 4231.')

    # Hash vectors.
    rows = []
    for alg, fn in SHA_ALGS:
        for title, msg in SHA_MESSAGES:
            digest = fn(msg).digest()
            if alg == 'SHA256' and msg == b'abc':
                check('FIPS 180 SHA-256 "abc"', digest,
                      'ba7816bf8f01cfea414140de5dae2223'
                      'b00361a396177a9cb410ff61f20015ad')
            rows.append([cstr('FIPS 180-4 %s %s' % (alg, title)),
                         'CRY_ALG_%s' % alg] + buf(e, msg) + buf(e, digest))
    e.table('xcry_hash_vector_t', 'xcry_sha_vectors', rows,
            'SHA-1 and SHA-2, FIPS 180-4 examples.')
    rows = []
    for alg, fn in SHA_ALGS:
        digest = fn(MILLION_A).digest()
        if alg == 'SHA256':
            check('FIPS 180 SHA-256 million a', digest,
                  'cdc76e5c9914fb9281a1c7e284d73e67'
                  'f1809a48a497200e046d39ccc7112cd0')
        rows.append([cstr('FIPS 180-4 %s one million "a"' % alg),
                     'CRY_ALG_%s' % alg, '%dU' % len(MILLION_A), 'NULL'] +
                    buf(e, digest))
    e.table('xcry_hash_vector_t', 'xcry_sha_million_a', rows,
            'SHA-1 and SHA-2 long message, the message is generated at '
            'runtime.')

    # Key derivation vectors.
    rows = []
    for title, ikm, salt, info, length in HKDF_CASES:
        okm = HKDF(hashes.SHA256(), length, salt or None, info).derive(ikm)
        if title == 'RFC 5869 TC1':
            check(title, okm, '3cb25f25faacd57a90434f64d0362f2a'
                              '2d2d0a90cf1a5a4c5db02d56ecc4c5bf'
                              '34007208d5b887185865')
        rows.append([cstr(title), 'CRY_ALG_HKDF_SHA256'] + buf(e, ikm) +
                     buf(e, salt) + buf(e, info) + buf(e, okm))
    e.table('xcry_kdf_vector_t', 'xcry_hkdf_vectors', rows,
            'HKDF-SHA256, RFC 5869.')

    os.makedirs(OUTDIR, exist_ok=True)
    with open(os.path.join(OUTDIR, 'xcry_vectors.c'), 'w') as f:
        f.write(HEADER_LICENSE + C_PREAMBLE + '\n'.join(e.arrays) + '\n' +
                '\n'.join(e.tables) + C_POSTAMBLE)
    with open(os.path.join(OUTDIR, 'xcry_vectors.h'), 'w') as f:
        f.write(HEADER_LICENSE + H_PREAMBLE + '\n'.join(e.decls) + '\n' +
                H_POSTAMBLE)


HEADER_LICENSE = '''/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
'''

C_PREAMBLE = '''
/**
 * @file    xcry_vectors.c
 * @brief   XHAL Crypto test suite vectors.
 * @note    Generated by test/xcrypto/ref/xcry_gen_vectors.py, do not edit.
 */

#include "hal.h"
#include "xcry_vectors.h"

'''

C_POSTAMBLE = ''

H_PREAMBLE = '''
/**
 * @file    xcry_vectors.h
 * @brief   XHAL Crypto test suite vectors.
 * @note    Generated by test/xcrypto/ref/xcry_gen_vectors.py, do not edit.
 */

#ifndef XCRY_VECTORS_H
#define XCRY_VECTORS_H

/**
 * @brief   Cipher vector.
 */
typedef struct {
  const char                *name;
  cry_algorithm_t           algorithm;
  size_t                    key_size;
  const uint8_t             *key;
  size_t                    iv_size;
  const uint8_t             *iv;
  size_t                    size;
  const uint8_t             *pt;
  const uint8_t             *ct;
} xcry_cipher_vector_t;

/**
 * @brief   AEAD vector.
 */
typedef struct {
  const char                *name;
  cry_algorithm_t           algorithm;
  size_t                    key_size;
  const uint8_t             *key;
  size_t                    nonce_size;
  const uint8_t             *nonce;
  size_t                    aad_size;
  const uint8_t             *aad;
  size_t                    size;
  const uint8_t             *pt;
  const uint8_t             *ct;
  size_t                    tag_size;
  const uint8_t             *tag;
} xcry_aead_vector_t;

/**
 * @brief   MAC vector.
 */
typedef struct {
  const char                *name;
  cry_algorithm_t           algorithm;
  size_t                    key_size;
  const uint8_t             *key;
  size_t                    size;
  const uint8_t             *msg;
  size_t                    tag_size;
  const uint8_t             *tag;
} xcry_mac_vector_t;

/**
 * @brief   Hash vector, a NULL message is generated by the test.
 */
typedef struct {
  const char                *name;
  cry_algorithm_t           algorithm;
  size_t                    size;
  const uint8_t             *msg;
  size_t                    digest_size;
  const uint8_t             *digest;
} xcry_hash_vector_t;

/**
 * @brief   Key derivation vector.
 */
typedef struct {
  const char                *name;
  cry_algorithm_t           algorithm;
  size_t                    ikm_size;
  const uint8_t             *ikm;
  size_t                    salt_size;
  const uint8_t             *salt;
  size_t                    info_size;
  const uint8_t             *info;
  size_t                    size;
  const uint8_t             *okm;
} xcry_kdf_vector_t;

'''

H_POSTAMBLE = '''
#endif /* XCRY_VECTORS_H */
'''

if __name__ == '__main__':
    main()
