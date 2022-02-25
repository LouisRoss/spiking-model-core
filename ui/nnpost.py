#!/usr/bin/python3

import sys
import subprocess
from pathlib import Path

from mes import configuration
import nnclean
import nnplot
import nntidy

python_interpreter = 'python'

def load_configuration():
    conf = configuration()
    if 'PostProcessing' not in conf.configuration:
        print('Required "PostProcessing" section not in configuration file')
        return

    if 'RecordLocation' not in conf.configuration['PostProcessing']:
        print('Required subkey "RecordLocation" not in configuration file "PostProcessing" section')
        return

    record_path = conf.configuration['PostProcessing']['RecordLocation']
    Path(record_path).mkdir(parents=True, exist_ok=True)
    return conf


def run():
    conf = load_configuration()
    nnclean.execute(conf)   # Clean the record data into a clean file.
    nnplot.execute(conf)    # Plot the cleaned data into a file.
    nntidy.execute(conf)    # Archive the record data and plot(s).

run()
