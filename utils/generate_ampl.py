#!/usr/bin/env python3
"""
Amplitude Generator Utility

Generates a CSV file with random amplitudes for configurable number of channels
and amplitude ranges.
"""

import csv
import random
import argparse
from typing import Tuple


def generate_amplitudes_csv(
    filename: str = "amplitudes.csv",
    num_channels: int = 10,
    amplitude_range: Tuple[float, float] = (0.0, 1000.0),
    decimal_places: int = 1
) -> None:
    """
    Generate a CSV file with random amplitudes for each channel.
    
    Args:
        filename: Output CSV filename
        num_channels: Number of channels to generate
        amplitude_range: Tuple of (min_amplitude, max_amplitude)
        decimal_places: Number of decimal places for amplitude values
    """
    min_amp, max_amp = amplitude_range
    
    with open(filename, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        
        # Write header
        writer.writerow(['#channel', 'amplitude'])
        
        # Generate random amplitudes for each channel
        for channel in range(1, num_channels + 1):
            amplitude = random.uniform(min_amp, max_amp)
            amplitude = round(amplitude, decimal_places)
            writer.writerow([channel, amplitude])
    
    print(f"Generated {filename} with {num_channels} channels")
    print(f"Amplitude range: {min_amp} to {max_amp}")


def main():
    """Command line interface for the amplitude generator."""
    parser = argparse.ArgumentParser(
        description="Generate CSV file with random channel amplitudes"
    )
    
    parser.add_argument(
        '-f', '--filename',
        default='amplitudes.csv',
        help='Output CSV filename (default: amplitudes.csv)'
    )
    
    parser.add_argument(
        '-n', '--channels',
        type=int,
        default=10,
        help='Number of channels (default: 10)'
    )
    
    parser.add_argument(
        '--min-amp',
        type=float,
        default=0.0,
        help='Minimum amplitude value (default: 0.0)'
    )
    
    parser.add_argument(
        '--max-amp',
        type=float,
        default=1000.0,
        help='Maximum amplitude value (default: 1000.0)'
    )
    
    parser.add_argument(
        '-d', '--decimals',
        type=int,
        default=1,
        help='Number of decimal places (default: 1)'
    )
    
    parser.add_argument(
        '--seed',
        type=int,
        help='Random seed for reproducible results'
    )
    
    args = parser.parse_args()
    
    # Set random seed if provided
    if args.seed is not None:
        random.seed(args.seed)
    
    # Generate the CSV
    generate_amplitudes_csv(
        filename=args.filename,
        num_channels=args.channels,
        amplitude_range=(args.min_amp, args.max_amp),
        decimal_places=args.decimals
    )


# Example usage as a module
if __name__ == "__main__":
    # You can also call the function directly with custom parameters
    # generate_amplitudes_csv(
    #     filename="my_amplitudes.csv",
    #     num_channels=20,
    #     amplitude_range=(50.0, 500.0),
    #     decimal_places=2
    # )
    
    # Or use the command line interface
    main()