#!/usr/bin/env python3
"""Packaging must reject missing, failed or stale validation evidence."""
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import TestCase, main
from unittest.mock import patch
import copy
import build_metadata as metadata

class ValidationEvidence(TestCase):
    def test_binding_and_failures(self):
        with TemporaryDirectory() as directory:
            root = Path(directory); binary = root / 'app'; binary.write_bytes(b'binary')
            flags = root / 'crengine/CMakeFiles/crengine.dir/flags.make'
            flags.parent.mkdir(parents=True); flags.write_text('flags')
            source = {'commit': 'abc', 'dirty': True, 'tree_sha256': 'tree'}
            report = {'status': 'passed', 'passed': sorted(metadata.REQUIRED_TESTS),
                      'binary_sha256': metadata.sha256(binary), 'source': source,
                      'configuration_sha256': metadata.sha256(flags)}
            with patch.object(metadata, 'source_metadata', return_value=source):
                self.assertEqual(metadata.verified_report(report, binary, root, root), report)
                for key, bad in [('status', 'failed'), ('passed', []), ('binary_sha256', 'wrong'),
                                 ('source', {}), ('configuration_sha256', 'wrong')]:
                    invalid = copy.deepcopy(report); invalid[key] = bad
                    with self.subTest(field=key), self.assertRaises(ValueError):
                        metadata.verified_report(invalid, binary, root, root)
                binary.write_bytes(b'changed binary')
                with self.assertRaises(ValueError): metadata.verified_report(report, binary, root, root)
if __name__ == '__main__': main()
