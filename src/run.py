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
        '--visits', type=int, default=50, help='Number of visits (default: 50)')
    parser.add_argument(
        '--playouts', type=int, default=0, help='Number of playouts (default: 0)')
    parser.add_argument(
        '--search', type=str, default='pucb', choices=['ucb1', 'pucb'],
        help='Criterion for selecting search nodes (default: pucb)')
    parser.add_argument(
        '--temperature', type=float, default=1.0, help='Temperature for exploration (default: 1.0)')
    parser.add_argument(
        '--randomness', type=float, default=0.0, help='Randomness for number of exploration (default: 0.0)')
    parser.add_argument(
        '--criterion', type=str, default='lcb', choices=['lcb', 'visits'],
        help='Criterion for candidate prioritization (default: lcb)')
    parser.add_argument(
        '--rule', type=str, default='ch', choices=['ch', 'jp', 'com'], help='Rule (default: ch)')
    parser.add_argument(
        '--boardsize', type=int, default=DEFAULT_SIZE, help=f'Board size (default: {DEFAULT_SIZE})')
    parser.add_argument(
        '--komi', type=float, default=DEFAULT_KOMI, help=f'Komi (default: {DEFAULT_KOMI})')
    parser.add_argument(
        '--superko', default=False, action='store_true', help='Use superko rule')
    parser.add_argument(
        '--eval-leaf-only', default=False, action='store_true', help='Evaluate leaf nodes only')
    parser.add_argument(
        '--timelimit', type=float, default=120, help='Timelimit (sec) (default: 120 sec)')
    parser.add_argument(
        '--ponder', default=False, action='store_true', help='Use pondering')
    parser.add_argument(
        '--resign', type=float, default=0.02, help='Resign threshold (default: 0.02)')
    parser.add_argument(
        '--min-score', type=float, default=0.0, help='Minium score at resign (default: 0.0)')
    parser.add_argument(
        '--min-turn', type=int, default=100, help='Minimum number of resign turns (default: 100)')
    parser.add_argument(
        '--initial-turn', type=int, default=4, help='Number of turns to move randomly (default: 4)')
    parser.add_argument(
        '--client-name', type=str, default=NAME, help=f'Client name (default: {NAME})')
    parser.add_argument(
        '--client-version', type=str, default=VERSION, help=f'Client version (default: {VERSION})')
    parser.add_argument(
        '--threads', type=int, default=16, help='Number of threads (default: 16)')
    parser.add_argument(
        '--display', type=str, default=None, help='Command to display board (default: None)')
    parser.add_argument(
        '--sgf', type=str, default=None, help='SGF file to load (default: None)')
    parser.add_argument(
        '--batch-size', type=int, default=2048, help='Batch size (default: 2048)')
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

    # Set up log output
    start_logging(debug=args.verbose, console=sys.stderr)

    # Configure GPU settings
    if torch.cuda.device_count() != 0:
        torch.backends.cudnn.enabled = True
        torch.backends.cudnn.benchmark = True
        torch.backends.cudnn.deterministic = False

    # Check game rules
    if args.rule == 'ch':
        rule = RULE_CH
    elif args.rule == 'jp':
        rule = RULE_JP
    elif args.rule == 'com':
        rule = RULE_COM
    else:
        raise ValueError(f'Invalid rule: {args.rule}')

    # Create inference object
    processor = Processor(args.model, args.gpus, args.batch_size, args.fp16)

    # Create GPT object
    engine = GTPEngine(
        processor=processor,
        threads=args.threads,
        visits=args.visits,
        playouts=args.playouts,
        use_ucb1=(args.search == 'ucb1'),
        temperature=args.temperature,
        randomness=args.randomness,
        criterion=args.criterion,
        rule=rule,
        boardsize=args.boardsize,
        komi=args.komi,
        superko=args.superko,
        eval_leaf_only=args.eval_leaf_only,
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

    # Load sgf file if specified
    if args.sgf is not None:
        engine.load(args.sgf)

    # Run the game
    engine.run()


if __name__ == '__main__':
    main()
