#!/usr/bin/env python3
"""Standalone tutor tests: no paid service, model download, or solver run."""
import argparse
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('fkpt2_ia_tutor', ROOT / 'scripts/ia_tutor.py')
tutor = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = tutor
spec.loader.exec_module(tutor)
EXECUTABLE = None


class TutorTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='fkpt2-tutor-')
        self.root = Path(self.temporary.name)
        (self.root / 'docs').mkdir()
        (self.root / 'docs/gsm.md').write_text(
            '# Gaussian Streaming Model\nUse gsm=true and clpt_b1 for Lagrangian bias.\n'
            'In Python call fkpt2.compute(gsm=True).\n', encoding='utf-8')
        (self.root / 'README.md').write_text('# Installation\nRun make all fkpt2py.\n', encoding='utf-8')
        with patch.dict(os.environ, {'FKPT2_TUTOR_BACKEND': '', 'FKPT2_TUTOR_MODEL': ''}):
            self.args = tutor.parser().parse_args(['--root', str(self.root)])

    def tearDown(self):
        self.temporary.cleanup()

    def test_recursive_readmes_and_documentation_exclusions(self):
        (self.root / 'addons/example').mkdir(parents=True)
        (self.root / 'addons/example/Readme.txt').write_text('Additional gravity model', encoding='utf-8')
        for folder in ['docs/_build', 'build', 'Output_test', '.git']:
            (self.root / folder).mkdir(exist_ok=True)
            (self.root / folder / 'README.md').write_text('Generated junk', encoding='utf-8')
        outside = self.root / 'private.txt'
        outside.write_text('Private data', encoding='utf-8')
        (self.root / 'docs/linked.md').symlink_to(outside)
        knowledge = tutor.Knowledge(self.root)
        self.assertEqual(set(knowledge.sources), {'README.md', 'docs/gsm.md', 'addons/example/Readme.txt'})
        passages = knowledge.search('GSM Python clpt_b1')
        self.assertEqual(passages[0].source, 'docs/gsm.md')
        self.assertIn('L1', passages[0].citation)
        self.assertFalse(knowledge.search('unknownunmatchedtoken'))

    def test_live_document_changes(self):
        path = self.root / 'docs/gsm.md'
        path.write_text('Updated uniquetermgravity', encoding='utf-8')
        self.assertEqual(tutor.Knowledge(self.root).search('uniquetermgravity')[0].source, 'docs/gsm.md')

    def test_exact_parameter_definitions_and_aliases_precede_related_prose(self):
        (self.root / 'include').mkdir()
        (self.root / 'include/run_parameters.def').write_text(
            'PARAMETER(b1, b1, "1", "Linear Eulerian bias")\n'
            'PARAMETER(clpt_b1, clpt_b1, "0", "Local Lagrangian bias")\n'
            'PARAMETER_ALIAS(s2eft, s2eft, "0",\n'
            '                "Dispersion coefficient", sigma2eft)\n', encoding='utf-8')
        knowledge = tutor.Knowledge(self.root)
        passage = knowledge.search('Compare b1 and clpt_b1')[0]
        self.assertEqual(passage.source, 'include/run_parameters.def')
        self.assertIn('Linear Eulerian bias', passage.text)
        self.assertIn('Local Lagrangian bias', passage.text)
        self.assertIn('L1-L1,L2-L2', passage.citation)
        self.assertIn('Dispersion coefficient', knowledge.search('sigma2eft')[0].text)

    def test_chat_import_and_historical_provenance(self):
        raw = self.root / 'conversation.json'
        raw.write_text(json.dumps({'title': 'GSM discussion', 'messages': [
            {'role': 'system', 'content': 'Do not include this instruction'},
            {'role': 'tool', 'content': 'Do not include tool data'},
            {'role': 'user', 'content': 'What was our special emulator decision?'},
            {'role': 'assistant', 'content': 'Use uniquedecisionmarker.'}]}), encoding='utf-8')
        directory = self.root / '.fkpt2-tutor/chats'
        path = tutor.import_chat(raw, directory)
        data = json.loads(path.read_text())
        self.assertEqual(len(data['messages']), 2)
        self.assertEqual(path.stat().st_mode & 0o777, 0o600)
        passages = tutor.Knowledge(self.root, [directory]).search('uniquedecisionmarker')
        self.assertTrue(passages[0].historical)
        self.assertIn('message-2', passages[0].citation)
        self.assertFalse(tutor.Knowledge(self.root).search('uniquedecisionmarker'))
        self.assertTrue(raw.exists())

    def test_chatgpt_export_current_branch(self):
        data = {'current_node': 'answer', 'mapping': {
            'question': {'parent': None, 'message': {'author': {'role': 'user'},
                        'content': {'parts': ['How does CLPT work?']}}},
            'answer': {'parent': 'question', 'message': {'author': {'role': 'assistant'},
                       'content': {'parts': ['Current answer']}}},
            'old': {'parent': 'question', 'message': {'author': {'role': 'assistant'},
                    'content': {'parts': ['Abandoned branch']}}}}}
        self.assertEqual(list(tutor.chat_messages(data)), [('user', 'How does CLPT work?'),
                                                          ('assistant', 'Current answer')])

    def test_no_chats_excludes_private_history_and_reviewed_notes(self):
        directory = self.root / '.fkpt2-tutor/chats'
        directory.mkdir(parents=True)
        (directory / 'history.txt').write_text('privateuniquemarker', encoding='utf-8')
        (self.root / 'tutor/notes').mkdir(parents=True)
        (self.root / 'tutor/notes/review.md').write_text('revieweduniquemarker', encoding='utf-8')
        result = subprocess.run([sys.executable, str(ROOT / 'scripts/ia_tutor.py'),
                                 '--root', str(self.root), '--no-chats', '--json',
                                 '--ask', 'privateuniquemarker revieweduniquemarker'],
                                text=True, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['sources'], [])

    def test_pdf_cache_content_hash_and_page_citations(self):
        path = self.root / 'docs/paper.pdf'
        path.write_bytes(b'fake PDF for extractor fixture')
        with patch.object(tutor.shutil, 'which', return_value='/mock/pdftotext'), \
             patch.object(tutor.subprocess, 'run', return_value=subprocess.CompletedProcess(
                 [], 0, 'Page one clptpdf\fPage two gsmpdf\f', '')) as run:
            knowledge = tutor.Knowledge(self.root)
        self.assertEqual(run.call_count, 1)
        self.assertIn('page-2', knowledge.search('gsmpdf')[0].citation)
        with patch.object(tutor.shutil, 'which', side_effect=AssertionError('cache should be used')):
            self.assertIn('docs/paper.pdf', tutor.Knowledge(self.root).sources)
        path.write_bytes(b'changed PDF')
        with patch.object(tutor.shutil, 'which', return_value=None), \
             patch.dict(sys.modules, {'pypdf': None}):
            knowledge = tutor.Knowledge(self.root)
        self.assertTrue(any('PDF not indexed' in warning for warning in knowledge.warnings))
        self.assertNotIn('docs/paper.pdf', knowledge.sources)

    def test_offline_has_no_network_and_unknown_question(self):
        self.args.backend = 'offline'
        knowledge = tutor.Knowledge(self.root)
        with patch.object(tutor, 'build_opener', side_effect=AssertionError('offline network call')):
            result = tutor.answer('How do I use GSM?', knowledge, [], self.args)
            empty = tutor.answer('unknownunmatchedtoken', knowledge, [], self.args)
        self.assertEqual(result['backend'], 'offline')
        self.assertIn('Documentation excerpts', result['answer'])
        self.assertTrue(result['sources'])
        self.assertIn('No matching evidence', empty['answer'])
        self.assertFalse(empty['sources'])

    def test_ollama_protocol_and_fallback(self):
        knowledge = tutor.Knowledge(self.root)
        response = io.BytesIO(json.dumps({'message': {'content': 'Use GSM [1].'}}).encode())
        with patch.object(tutor, 'build_opener') as opener:
            opener.return_value.open.return_value = response
            result = tutor.answer('GSM', knowledge, [('Previous question', 'Previous answer')], self.args)
        request = opener.return_value.open.call_args[0][0]
        payload = json.loads(request.data)
        self.assertEqual(request.full_url, 'http://127.0.0.1:11434/api/chat')
        self.assertEqual(payload['model'], 'qwen3.5:4b')
        self.assertFalse(payload['stream'])
        self.assertFalse(payload['think'])
        self.assertIn('docs/gsm.md', payload['messages'][0]['content'])
        self.assertEqual(payload['messages'][-1]['content'], 'GSM')
        self.assertEqual(result['backend'], 'ollama')
        self.assertEqual(result['answer'], 'Use GSM [1].')
        for raw in [b'[]', b'{}', b'{"error":"model missing"}', b'not JSON',
                    b'{"message":{"content":""}}']:
            with patch.object(tutor, 'build_opener') as opener:
                opener.return_value.open.return_value = io.BytesIO(raw)
                result = tutor.answer('GSM', knowledge, [], self.args)
            self.assertEqual(result['backend'], 'offline')
            self.assertTrue(any('Ollama unavailable' in w for w in result['warnings']))
        with patch.object(tutor, 'build_opener', side_effect=OSError('connection refused')):
            result = tutor.answer('GSM', knowledge, [], self.args)
        self.assertIn('ollama pull qwen3.5:4b', result['warnings'][-1])

    def test_local_endpoint_validation_and_redirects(self):
        for url in ['http://127.0.0.1:11434', 'http://localhost:11434/', 'http://[::1]:11434']:
            self.assertTrue(tutor.ollama_endpoint(url).endswith('/api/chat'))
        for url in ['https://ollama.com', 'http://192.168.1.1:11434', 'file:///tmp/a',
                    'http://localhost:bad', 'http://user:pass@localhost:11434',
                    'http://localhost:11434/other', 'http://localhost:11434/?token=private']:
            with self.assertRaises(ValueError, msg=url):
                tutor.ollama_endpoint(url)
        with self.assertRaises(ValueError):
            tutor.NoRedirect().redirect_request(None, None, 302, '', None, 'https://remote')

    def test_native_launch_from_another_directory_and_no_solver_output(self):
        if EXECUTABLE is None:
            self.skipTest('pass --fkpt2 to exercise the native launcher')
        env = os.environ.copy()
        env['FKPT2_TUTOR_ROOT'] = str(ROOT)
        env['FKPT2_TUTOR_PYTHON'] = sys.executable
        env['FKPT2_TUTOR_BACKEND'] = 'offline'
        result = subprocess.run([str(EXECUTABLE), 'options=IA-tutor', '--root', str(self.root),
                                 '--ask', 'GSM Python', '--json'], cwd=self.root, env=env,
                                text=True, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        data = json.loads(result.stdout)
        self.assertEqual(data['backend'], 'offline')
        self.assertEqual(data['sources'][0]['source'], 'docs/gsm.md')
        self.assertFalse((self.root / 'Output').exists())
        for arguments in [['options=IA-tutor,make-info'], ['options=IA-tutor', 'Nk=10'],
                          ['options=IA-tutor', '--ask', 'GSM', '--top-k', '0']]:
            result = subprocess.run([str(EXECUTABLE), *arguments], cwd=self.root, env=env,
                                    text=True, capture_output=True, timeout=30)
            self.assertNotEqual(result.returncode, 0, result.stdout)
        env['FKPT2_TUTOR_PYTHON'] = '/missing/python'
        result = subprocess.run([str(EXECUTABLE), 'options=IA-tutor', '--help'], env=env,
                                text=True, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 127)
        self.assertIn('FKPT2_TUTOR_PYTHON', result.stderr)
        env['FKPT2_TUTOR_PYTHON'] = sys.executable
        env['OMPI_COMM_WORLD_SIZE'] = '2'
        result = subprocess.run([str(EXECUTABLE), 'options=IA-tutor', '--help'], env=env,
                                text=True, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 2)
        self.assertIn('serial conversation', result.stderr)

    def test_interactive_session_and_private_save(self):
        result = subprocess.run([sys.executable, str(ROOT / 'scripts/ia_tutor.py'),
                                 '--root', str(self.root), '--backend', 'offline'],
                                input='/backend ollama\n/backend offline\nGSM Python\n/save\n/reset\n/quit\n', text=True,
                                capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('docs/gsm.md', result.stdout)
        self.assertIn('Backend: ollama', result.stdout)
        self.assertIn('Backend: offline', result.stdout)
        self.assertEqual(len(list((self.root / '.fkpt2-tutor/chats').glob('*.json'))), 1)
        self.assertIn('Current conversation cleared', result.stdout)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--fkpt2', type=Path)
    args, rest = parser.parse_known_args()
    EXECUTABLE = args.fkpt2.resolve() if args.fkpt2 else None
    unittest.main(argv=[sys.argv[0], *rest])
