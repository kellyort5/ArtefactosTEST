"""Regression checks for CLI selection only; broker smoke uses real Docker/MQTT."""
import os
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch
import common

WINDOWS = '/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe'


class DockerSelectionTest(unittest.TestCase):
    def setUp(self):
        self.environment = patch.dict(os.environ, {}, clear=True)
        self.environment.start()
        self.addCleanup(self.environment.stop)
        common._automatic_docker_bin.cache_clear()
        self.addCleanup(common._automatic_docker_bin.cache_clear)
        self.native = patch('common.shutil.which', return_value='/usr/bin/docker')
        self.native.start()
        self.addCleanup(self.native.stop)
        self.exists = patch.object(Path, 'exists', return_value=True)
        self.exists.start()
        self.addCleanup(self.exists.stop)

    @patch('common.subprocess.run')
    def test_native_responsive_and_cached(self, run):
        run.return_value.returncode = 0
        self.assertEqual(common.docker_bin(), '/usr/bin/docker')
        self.assertEqual(common.docker_bin(), '/usr/bin/docker')
        self.assertEqual(run.call_count, 1)

    @patch('common.subprocess.run')
    def test_unresponsive_native_falls_back_and_converts_paths(self, run):
        run.side_effect = [subprocess.CompletedProcess([], 1), subprocess.CompletedProcess([], 0)]
        self.assertEqual(common.docker_bin(), WINDOWS)
        with patch('common.subprocess.check_output', return_value='C:\\lab\\broker\n') as convert:
            self.assertEqual(common.host_path(common.BROKER), 'C:\\lab\\broker')
            self.assertEqual(convert.call_args.args[0][:2], ['wslpath', '-w'])

    @patch('common.subprocess.run')
    def test_probe_timeout_falls_back(self, run):
        run.side_effect = [subprocess.TimeoutExpired('docker info', 5), subprocess.CompletedProcess([], 0)]
        self.assertEqual(common.docker_bin(), WINDOWS)

    @patch('common.subprocess.run')
    def test_explicit_binary_is_authoritative(self, run):
        os.environ['DOCKER_BIN'] = '/custom/docker'
        self.assertEqual(common.docker_bin(), '/custom/docker')
        run.assert_not_called()

    @patch('common.subprocess.run')
    def test_explicit_endpoint_never_selects_another_engine(self, run):
        os.environ['DOCKER_HOST'] = 'unix:///chosen/docker.sock'
        run.return_value.returncode = 1
        self.assertEqual(common.docker_bin(), '/usr/bin/docker')
        self.assertEqual(run.call_count, 1)

    @patch('common.subprocess.run')
    def test_both_unavailable_keep_cli_for_offline_config(self, run):
        run.return_value.returncode = 1
        self.assertEqual(common.docker_bin(), '/usr/bin/docker')
        self.assertEqual(run.call_count, 2)


if __name__ == '__main__':
    unittest.main()
