import argparse
import sys

import torch
from deepgo.config import (DEFAULT_KOMI, DEFAULT_SIZE, NAME, RULE_CH, RULE_COM,
                           RULE_JP, VERSION)
from deepgo.gpu import get_default_gpus
from deepgo.gtp import GTPEngine
from deepgo.log import start_logging
from deepgo.processor import Processor


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Run with GTP mode.')
    parser.add_argument(
        'model', type=str, help='Path to the model file')
    parser.add_argument(
        '--visits', type=int, default=50, help='Number of visits')
    parser.add_argument(
        '--search', type=str, default='pucb', choices=['ucb1', 'pucb'],
        help='Calculation method of search (default: pucb)')
    parser.add_argument(
        '--rule', type=str, default='ch', choices=['ch', 'jp', 'com'], help='Rule')
    parser.add_argument(
        '--boardsize', type=int, default=DEFAULT_SIZE, help=f'Board size (default: {DEFAULT_SIZE})')
    parser.add_argument(
        '--komi', type=float, default=DEFAULT_KOMI, help=f'Komi (default: {DEFAULT_KOMI})')
    parser.add_argument(
        '--superko', default=False, action='store_true', help='Use superko rule')
    parser.add_argument(
        '--timelimit', type=float, default=10, help='Timelimit (sec) (default: 10 sec)')
    parser.add_argument(
        '--ponder', default=False, action='store_true', help='Use pondering')
    parser.add_argument(
        '--resign', type=float, default=0.02, help='Resign threshold (default: 0.02)')
    parser.add_argument(
        '--min-score', type=float, default=0.0, help='Minium score at resign (default: 0.0)')
    parser.add_argument(
        '--min-turn', type=int, default=100, help='Minimum number of resign turns (default: 100)')
    parser.add_argument(
        '--initial-turn', type=int, default=0, help='Number of turns to move randomly (default: 0)')
    parser.add_argument(
        '--client-name', type=str, default=NAME, help='Client name (default: {NAME})')
    parser.add_argument(
        '--client-version', type=str, default=VERSION, help=f'Client version (default: {VERSION})')
    parser.add_argument(
        '--threads', type=int, default=16, help='Number of threads')
    parser.add_argument(
        '--display', type=str, default=None, help='Command to display board')
    parser.add_argument(
        '--sgf', type=str, default=None, help='SGF file to load')
    parser.add_argument(
        '--batch-size', type=int, default=2048, help='Batch size')
    parser.add_argument(
        '--gpus', type=lambda x: list(map(int, x.split(','))), default=None,
        help='GPU IDs (comma-separated) (default: all available GPUs)')
    parser.add_argument(
        '--fp16', default=False, action='store_true', help='Use FP16')
    parser.add_argument(
        '--verbose', action='store_true', help='Verbose mode')

    args = parser.parse_args()
    args.gpus, args.fp16 = get_default_gpus(args.gpus, args.fp16)

    return args


def main() -> None:
    args = parse_args()

    # ログ出力を設定する
    start_logging(debug=args.verbose, console=sys.stderr)

    # GPUに関する設定を行う
    if torch.cuda.device_count() != 0:
        torch.backends.cudnn.enabled = True
        torch.backends.cudnn.benchmark = True
        torch.backends.cudnn.deterministic = False

    # ゲームルールを確認する
    if args.rule == 'ch':
        rule = RULE_CH
    elif args.rule == 'jp':
        rule = RULE_JP
    elif args.rule == 'com':
        rule = RULE_COM
    else:
        raise ValueError(f'Invalid rule: {args.rule}')

    # 推論オブジェクトを作成する
    processor = Processor(args.model, args.gpus, args.batch_size, args.fp16)

    # GPTオブジェクトを作成する
    engine = GTPEngine(
        processor=processor,
        threads=args.threads,
        visits=args.visits,
        use_ucb1=(args.search == 'ucb1'),
        rule=rule,
        boardsize=args.boardsize,
        komi=args.komi,
        superko=args.superko,
        timelimit=args.timelimit,
        ponder=args.ponder,
        resign_threshold=args.resign,
        resign_score=args.min_score,
        resign_turn=args.min_turn,
        initial_turn=args.initial_turn,
        client_name=args.client_name,
        client_version=args.client_version,
        display=args.display,
    )

    # sgfファイルが指定されていれば読み込む
    if args.sgf is not None:
        engine.load(args.sgf)

    # ゲームを実行する
    engine.run()


if __name__ == '__main__':
    main()
